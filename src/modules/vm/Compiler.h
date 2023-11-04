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

	void SetRegKeep(int reg, bool keep);

	void ExpectRegFree();

	bool IsPrecompCondBranch() const { 
		return m_precomp_cond_branch; 
	}

private:
	struct Register
	{
		bool used = false;
		bool keep = false;
	};
	std::array<Register, REGISTER_COUNT> m_registers;

	bool m_precomp_cond_branch = false;

}; // Compiler

}