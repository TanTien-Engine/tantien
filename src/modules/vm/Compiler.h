#pragma once

#include <easyvm/VM.h>

#include <array>
#include <string>
#include <map>

namespace tt
{

class Compiler
{
public:
	int NewRegister();
	void FreeRegister(int reg);

	void SetRegKeep(int reg, bool keep);

	void StatCall(const std::string& name);

	void Finish();

	void AddCost(int cost) { m_cost += cost; }
	int GetCost() const { return m_cost; }

private:
	struct Register
	{
		bool used = false;
		bool keep = false;
	};
	std::array<Register, REGISTER_COUNT> m_registers;

	std::map<std::string, int> m_stat_call;

	int m_cost = 0;

}; // Compiler

}