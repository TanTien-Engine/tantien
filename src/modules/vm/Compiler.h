#pragma once

#include <easyvm/VM.h>

#include <array>

namespace tt
{

class Compiler
{
public:
	Compiler();

	int NewRegister();
	void FreeRegister(int reg);

private:
	std::array<bool, REGISTER_COUNT> m_registers;

}; // Compiler

}