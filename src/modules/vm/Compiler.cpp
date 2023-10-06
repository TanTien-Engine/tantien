#include "Compiler.h"

#include <assert.h>

namespace tt
{

Compiler::Compiler() 
{
    m_registers.fill(false);
}

int Compiler::NewRegister()
{
    for (int i = 0; i < REGISTER_COUNT; ++i)
    {
        if (!m_registers[i])
        {
            m_registers[i] = true;
            return i;
        }
    }
    return -1;
}

void Compiler::FreeRegister(int reg)
{
    assert(reg >= 0 && reg < REGISTER_COUNT);
    m_registers[reg] = false;
}

}