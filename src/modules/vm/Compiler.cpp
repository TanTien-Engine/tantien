#include "Compiler.h"

#include <assert.h>

#include <stdexcept>

namespace tt
{

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

void Compiler::Finish()
{
    for (auto r : m_registers) 
    {
        if (r.used) {
            throw std::runtime_error("Register leakage!");
        }
    }
}

}