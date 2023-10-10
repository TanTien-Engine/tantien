#pragma once

#include <easyvm/VM.h>

#include <array>
#include <string>

namespace tt
{

class Compiler
{
public:
	int NewRegister();
	void FreeRegister(int reg);

	void SetRegType(int reg, const std::string& type);
	std::string GetRegType(int reg) const;

	void SetRegKeep(int reg, bool keep);

private:
	struct Register
	{
		bool used = false;
		std::string type;
		bool keep = false;
	};
	std::array<Register, REGISTER_COUNT> m_registers;

}; // Compiler

}