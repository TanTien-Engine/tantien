#include "CodesOptimize.h"
#include "Bytecodes.h"
#include "OpFieldMap.h"

#include "math_opcodes.h"
#include "stl_opcodes.h"
#include "geo_opcodes.h"

#include "VM.h"

#include <easyvm/VM.h>

#include <algorithm>

#include <assert.h>

namespace tt
{

void CodesOptimize::AddBlock(size_t hash, int begin, int end, int reg)
{
    CodeBlock b;
    b.hash = hash;
    b.begin = begin;
    b.end = end;
    b.reg = reg;
    b.times = 1;

    auto itr = m_blocks.find(hash);
    if (itr == m_blocks.end())
    {
        m_blocks.insert({ hash, { b } });
    }
    else
    {
        assert(begin - end == itr->second.front().begin - itr->second.front().end);
        itr->second.push_back(b);
    }
}

void CodesOptimize::RmDupCodes(const std::shared_ptr<Bytecodes>& codes)
{
    auto blocks = PrepareBlocks();
    if (blocks.empty()) {
        return;
    }

    for (int i = 0, n = static_cast<int>(blocks.size()); i < n; ++i) {
        for (auto& b : blocks[i]) {
            b.group = i;
        }
    }

    auto& old_codes = codes->GetCode();

    auto vm = std::make_shared<evm::VM>((char*)old_codes.data(), old_codes.size());

    MathOpCodeImpl::OpCodeInit(vm.get());
    StlOpCodeImpl::OpCodeInit(vm.get());
    GeoOpCodeImpl::OpCodeInit(vm.get());

    auto cache = tt::VM::Instance()->GetCache();
    cache->Resize(blocks.size());

    for (int i = 0; i < blocks.size(); ++i)
    {
        auto& b = blocks[i].front();

        vm->Run(b.begin, b.end);

        cache->SetValue(i, vm->GetRegister(b.reg));
    }

    std::vector<CodeBlock> sorted;
    for (auto bs : blocks) {
        for (auto b : bs) {
            sorted.push_back(b);
        }
    }
    std::sort(sorted.begin(), sorted.end(), 
        [](const CodeBlock& a, const CodeBlock& b) 
    {
        return a.begin < b.begin;
    });

    std::vector<uint8_t> new_codes;

    int curr_pos = 0;
    for (size_t i = 0, n = sorted.size(); i < n; ++i)
    {
        auto& block = sorted[i];
        if (block.begin < curr_pos) {
            assert(0);
            continue;
        }
        std::copy(old_codes.begin() + curr_pos, old_codes.begin() + block.begin, std::back_inserter(new_codes));
        curr_pos = block.end;

        new_codes.push_back(OP_POLY_COPY_FROM_MEM);
        new_codes.push_back(block.reg);
        new_codes.push_back(block.group);
    }

    VM::Instance()->GetOpFields()->Add(OP_POLY_COPY_FROM_MEM, 
        { OpFieldType::OpType, OpFieldType::Reg, OpFieldType::Reg }
    );

    std::copy(old_codes.begin() + curr_pos, old_codes.end(), std::back_inserter(new_codes));

    codes->SetCode(new_codes);
}

std::vector<std::vector<CodeBlock>> CodesOptimize::PrepareBlocks() const
{
    std::vector<std::vector<CodeBlock>> blocks;
    for (auto b : m_blocks) {
        if (b.second.size() > 1) {
            blocks.push_back(b.second);
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

}