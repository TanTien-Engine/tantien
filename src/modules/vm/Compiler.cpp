#include "Compiler.h"

#include <assert.h>

namespace tt
{

int Compiler::NewRegister()
{
    for (int i = 0; i < REGISTER_COUNT; ++i)
    {
        if (!m_registers[i].used)
        {
            m_registers[i].used = true;
            m_registers[i].type.clear();
            return i;
        }
    }
    return -1;
}

void Compiler::FreeRegister(int reg)
{
    assert(reg >= 0 && reg < REGISTER_COUNT);
    m_registers[reg].used = false;
    m_registers[reg].type.clear();
}

void Compiler::SetRegType(int reg, const std::string& type)
{
    assert(reg >= 0 && reg < REGISTER_COUNT);
    m_registers[reg].type = type;
}

std::string Compiler::GetRegType(int reg) const
{
    assert(reg >= 0 && reg < REGISTER_COUNT);
    return m_registers[reg].type;
}

}