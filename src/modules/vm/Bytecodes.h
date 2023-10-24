#pragma once

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

	auto& GetCode() const { return m_code; }
	void SetCode(const std::vector<uint8_t>& code) { m_code = code; }

	void SetRetReg(int reg) { m_ret_reg = reg; }
	int GetRetReg() const { return m_ret_reg; }

	void SetCurrPos(int pos) { m_curr_pos = pos; }

	// optimize
	void StatCall(const std::string& name);
	void AddCost(int cost) { m_cost += cost; }
	int GetCost() const { return m_cost; }
	auto GetOptimizer() const { return m_optimize; }

private:
	std::vector<uint8_t> m_code;

	int m_ret_reg = -1;

	int m_curr_pos = -1;

	// optimize
	std::map<std::string, int> m_stat_call;
	int m_cost = 0;
	std::shared_ptr<CodesOptimize> m_optimize;

}; // Bytecodes

}