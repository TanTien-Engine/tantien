#pragma once

#include <vector>
#include <iterator>

namespace tt
{

class Bytecodes
{
public:
	void Write(const char* data, size_t size) { 
		std::copy(data, data + size, std::back_inserter(m_code));
	}

	auto& GetCode() const { return m_code; }

	void SetRetReg(int reg) { m_ret_reg = reg; }
	int GetRetReg() const { return m_ret_reg; }

private:
	std::vector<uint8_t> m_code;

	int m_ret_reg = -1;

}; // Bytecodes

}