#pragma once

#include <vector>
#include <iterator>

namespace tt
{

class Bytecodes
{
public:
	void WriteType(uint8_t op);
	void WriteReg(uint8_t reg);
	void WriteDouble(double d);
	void WriteFloat(float f);
	void WriteInt(int i);
	void WriteBool(bool b);

	auto& GetCode() const { return m_code; }

	void SetRetReg(int reg) { m_ret_reg = reg; }
	int GetRetReg() const { return m_ret_reg; }

	void SetCurrPos(int pos) { m_curr_pos = pos; }

private:
	void Write(const char* data, size_t size);

private:
	std::vector<uint8_t> m_code;

	int m_ret_reg = -1;

	int m_curr_pos = -1;

}; // Bytecodes

}