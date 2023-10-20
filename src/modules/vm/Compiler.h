#pragma once

#include <easyvm/VM.h>

#include <array>
#include <string>
#include <map>

namespace tt
{

class Bytecodes;
class CodesOptimize;

class Compiler
{
public:
	Compiler();

	int NewRegister();
	void FreeRegister(int reg);

	void SetRegKeep(int reg, bool keep);

	void StatCall(const std::string& name);

	void AddCodeBlock(size_t hash, int begin, int end, int reg);

	void Finish(const std::shared_ptr<Bytecodes>& codes);

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

	std::shared_ptr<CodesOptimize> m_optim;

}; // Compiler

}