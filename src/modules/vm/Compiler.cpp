#include "Compiler.h"
#include "CodesOptimize.h"

#include <stdexcept>

#include <assert.h>

namespace tt
{

Compiler::Compiler()
{
    m_optim = std::make_shared<CodesOptimize>();
}

int Compiler::NewRegister()
{
    for (int i = 0; i < REGISTER_COUNT; ++i)
    {
        if (!m_registers[i].used)
        {
            m_registers[i].used = true;
            return i;
        }
    }
    return -1;
}

void Compiler::FreeRegister(int reg)
{
    assert(reg >= 0 && reg < REGISTER_COUNT);

    if (m_registers[reg].keep) {
        return;
    }

    m_registers[reg].used = false;
}

void Compiler::SetRegKeep(int reg, bool keep)
{
    assert(reg >= 0 && reg < REGISTER_COUNT);
    m_registers[reg].keep = keep;
}

void Compiler::StatCall(const std::string& name)
{
    auto itr = m_stat_call.find(name);
    if (itr == m_stat_call.end()) {
        m_stat_call.insert({ name, 1 });
    } else {
        ++itr->second;
    }
}

void Compiler::AddCodeBlock(size_t hash, int begin, int end, int reg)
{
    m_optim->AddBlock(hash, begin, end, reg);
}

void Compiler::Finish(const std::shared_ptr<Bytecodes>& codes)
{
    m_optim->RmDupCodes(codes);

    for (auto r : m_registers)
    {
        if (r.used) {
            throw std::runtime_error("Register leakage!");
        }
    }
}

}