#pragma once

#include "CodeBlock.h"

#include <vector>
#include <iterator>
#include <string>
#include <map>
#include <memory>

namespace tt
{

class CodesOptimize;

class Bytecodes
{
public:
	Bytecodes();

	void Write(const char* data, size_t size);

	// rewrite
	void WriteNum(int pos, float num);
	void WriteFlush();

	// optimize
	void AddOptimBlock(size_t hash, int begin, int end, int reg);
	std::shared_ptr<Bytecodes> RmDupCodes(const std::shared_ptr<Bytecodes>& codes);

	auto& GetCode() const { return m_code; }
	void SetCode(const std::vector<uint8_t>& code) { m_code = code; }

	void SetRetReg(int reg) { m_ret_reg = reg; }
	int GetRetReg() const { return m_ret_reg; }

	void SetCurrPos(int pos) { m_curr_pos = pos; }

	// statistics
	void StatCall(const std::string& name);
	void AddCost(int cost) { m_cost += cost; }
	int GetCost() const { return m_cost; }

private:
	std::vector<uint8_t> m_code;

	int m_ret_reg = -1;

	int m_curr_pos = -1;

	// statistics
	std::map<size_t, std::vector<CodeBlock>> m_blocks;

	// optimize
	std::map<std::string, int> m_stat_call;
	int m_cost = 0;
	mutable std::shared_ptr<CodesOptimize> m_optimize = nullptr;

}; // Bytecodes

}