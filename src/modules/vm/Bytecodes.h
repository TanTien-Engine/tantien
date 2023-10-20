#pragma once

#include <vector>
#include <iterator>

namespace tt
{

class Bytecodes
{
public:
	void Write(const char* data, size_t size);

	auto& GetCode() const { return m_code; }
	void SetCode(const std::vector<uint8_t>& code) { m_code = code; }

	void SetRetReg(int reg) { m_ret_reg = reg; }
	int GetRetReg() const { return m_ret_reg; }

	void SetCurrPos(int pos) { m_curr_pos = pos; }

private:
	std::vector<uint8_t> m_code;

	int m_ret_reg = -1;

	int m_curr_pos = -1;

}; // Bytecodes

}