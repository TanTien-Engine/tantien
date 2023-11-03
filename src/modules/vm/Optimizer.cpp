#include "Optimizer.h"
#include "Bytecodes.h"
#include "OpFieldMap.h"
#include "VM.h"
#include "geo_opcodes.h"
#include "Decompiler.h"

#include <easyvm/VM.h>

#include <algorithm>
#include <thread>

#include <assert.h>

//#define ONLY_CACHE_DUP_BLOCKS

namespace tt
{

Optimizer::Optimizer(const std::shared_ptr<Bytecodes>& old_codes)
    : m_old_codes(old_codes)
    , m_new_codes(nullptr)
{
}

void Optimizer::Optimize()
{
    CacheBlocks();
}

std::vector<std::vector<CodeBlock>> 
Optimizer::CalcBlocks() const
{
    auto& _blocks = m_old_codes->GetCodeBlocks();

    std::vector<std::vector<CodeBlock>> blocks;
    for (auto& bs : _blocks) 
    {
#ifdef ONLY_CACHE_DUP_BLOCKS
        if (bs.second.size() > 1) 
#endif // ONLY_CACHE_DUP_BLOCKS
        {
            blocks.push_back(bs.second);
        }
    }

    if (blocks.size() <= 1) {
        return blocks;
    }

    std::sort(blocks.begin(), blocks.end(), 
        [](const std::vector<CodeBlock>& a, const std::vector<CodeBlock>& b) 
    {
        int a_sz = a.front().end - a.front().begin;
        int b_sz = b.front().end - b.front().begin;
        return a_sz > b_sz;
    });

    for (int i = 1; i < blocks.size(); )
    {
        bool need_del = false;
        for (int j = 0; j < i; ++j)
        {
            if (blocks[i].size() == blocks[j].size())
            {
                bool all_in = true;
                for (int k = 0; k < blocks[i].size(); ++k)
                {
                    auto& bi = blocks[i][k];
                    auto& bj = blocks[j][k];
                    if (bi.begin < bj.begin || bi.end > bj.end) {
                        all_in = false;
                    }
                }
                if (all_in) {
                    need_del = true;
                    break;
                }
            }
        }
        if (need_del) {
            blocks.erase(blocks.begin() + i);
        } else {
            ++i;
        }
    }

    return blocks;
}

void Optimizer::CacheBlocks() const
{
    auto blocks = CalcBlocks();
    if (blocks.empty()) {
        return;
    }

    m_cached_blocks = blocks;

    for (int i = 0, n = static_cast<int>(blocks.size()); i < n; ++i) {
        for (auto& b : blocks[i]) {
            b.group = i;
        }
    }

    auto& old_codes = m_old_codes->GetCode();

    auto cache = tt::VM::Instance()->GetCache();
    cache->Resize(blocks.size());

    if (blocks.size() >= 2)
    {
        std::vector<std::thread> ts;
        for (int i = 0; i < blocks.size(); ++i)
        {
            ts.push_back(std::thread([](const CodeBlock& b, int i, const std::vector<uint8_t>& old_codes, const std::shared_ptr<ValueCache>& cache)
            {
                auto vm = tt::VM::Instance()->CreateVM(old_codes);
                vm->Run(b.begin, b.end);
                cache->SetValue(i, vm->GetRegister(b.reg));

            }, blocks[i].front(), i, old_codes, cache));
        }
        for (auto& t : ts) {
            t.join();
        }
    }
    else
    {
        auto vm = tt::VM::Instance()->CreateVM(old_codes);
        for (int i = 0; i < blocks.size(); ++i)
        {
            auto& b = blocks[i].front();
            vm->Run(b.begin, b.end);
            cache->SetValue(i, vm->GetRegister(b.reg));
        }
    }

    std::vector<CodeBlock> sorted;
    for (auto& bs : blocks) {
        for (auto& b : bs) {
            sorted.push_back(b);
        }
    }
    std::sort(sorted.begin(), sorted.end(), 
        [](const CodeBlock& a, const CodeBlock& b) 
    {
        if (a.begin == b.begin)
        {
            return a.end - a.begin > b.end - b.begin;
        }
        else
        {
            return a.begin < b.begin;
        }
    });

    auto tmp_codes = std::make_shared<Bytecodes>();
    tmp_codes->SetCode(old_codes);

    tt::Decompiler dc(tmp_codes, tt::VM::Instance()->GetOpFields());
    dc.JumpLabelRelocate(sorted);

    auto& fixed_old_codes = tmp_codes->GetCode();

    std::vector<uint8_t> new_codes;

    int curr_pos = 0;
    for (size_t i = 0, n = sorted.size(); i < n; ++i)
    {
        auto& block = sorted[i];
        if (block.begin < curr_pos) 
        {
            assert(block.end <= curr_pos);
            continue;
        }
        std::copy(fixed_old_codes.begin() + curr_pos, fixed_old_codes.begin() + block.begin, std::back_inserter(new_codes));
        curr_pos = block.end;

        new_codes.push_back(OP_POLY_COPY_FROM_MEM);
        new_codes.push_back(block.reg);
        new_codes.push_back(block.group);
    }

    VM::Instance()->GetOpFields()->Add(OP_POLY_COPY_FROM_MEM, 
        { OpFieldType::OpType, OpFieldType::Reg, OpFieldType::Reg }
    );

    std::copy(fixed_old_codes.begin() + curr_pos, fixed_old_codes.end(), std::back_inserter(new_codes));

    m_new_codes = std::make_shared<Bytecodes>();
    m_new_codes->SetCode(new_codes);
}

void Optimizer::WriteNumber(int pos, float num)
{
    // in rm codes
    for (size_t i = 0, n = m_cached_blocks.size(); i < n; ++i)
    {
        for (auto& b : m_cached_blocks[i])
        {
            if (pos < b.begin || pos >= b.end) {
                continue;
            }

            m_old_codes->SetCurrPos(pos);
            m_old_codes->Write(reinterpret_cast<const char*>(&num), sizeof(float));

            b.dirty = true;

            return;
        }
    }

    // outside
    if (m_new_codes)
    {
        m_new_codes->SetCurrPos(Relocate(pos));
        m_new_codes->Write(reinterpret_cast<const char*>(&num), sizeof(float));
    }
}

void Optimizer::FlushCache()
{
    int num = 0;

    for (size_t i = 0, n = m_cached_blocks.size(); i < n; ++i) {
        for (auto& b : m_cached_blocks[i]) {
            if (b.dirty) {
                ++num;
            }
        }
    }

    if (num == 0) {
        return;
    }

    if (num >= 2)
    {
        std::vector<std::thread> ts;

        for (size_t i = 0, n = m_cached_blocks.size(); i < n; ++i)
        {
            for (auto& b : m_cached_blocks[i])
            {
                if (!b.dirty) {
                    continue;
                }

                ts.push_back(std::thread([](const CodeBlock& b, size_t i, const std::vector<uint8_t>& old_codes)
                {
                    auto vm = tt::VM::Instance()->CreateVM(old_codes);

                    vm->Run(b.begin, b.end);

                    auto cache = tt::VM::Instance()->GetCache();
                    cache->SetValue(static_cast<int>(i), vm->GetRegister(b.reg));
                }, b, i, m_old_codes->GetCode()));

                b.dirty = false;
            }
        }

        for (auto& t : ts) {
            t.detach();
        }
    }
    else
    {
        auto vm = tt::VM::Instance()->CreateVM(m_old_codes->GetCode());
        auto cache = tt::VM::Instance()->GetCache();

        for (size_t i = 0, n = m_cached_blocks.size(); i < n; ++i)
        {
            for (auto& b : m_cached_blocks[i])
            {
                if (!b.dirty) {
                    continue;
                }

                vm->Run(b.begin, b.end);                
                cache->SetValue(static_cast<int>(i), vm->GetRegister(b.reg));

                b.dirty = false;
            }
        }
    }
}

int Optimizer::Relocate(int pos) const
{
    int offset = 0;
    for (auto& bs : m_cached_blocks)
    {
        for (auto& b : bs) 
        {
            if (pos >= b.end) 
            {
                // del code block
                offset += b.end - b.begin;
                // add OP_POLY_COPY_FROM_MEM
                offset -= 3;
            }
        }
    }
    return pos - offset;
}

}