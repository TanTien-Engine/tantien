#include "Bytecodes.h"

namespace tt
{

void Bytecodes::WriteType(uint8_t op)
{
	Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));
}

void Bytecodes::WriteReg(uint8_t reg)
{
	Write(reinterpret_cast<const char*>(&reg), sizeof(uint8_t));
}

void Bytecodes::WriteDouble(double d)
{
	Write(reinterpret_cast<const char*>(&d), sizeof(double));
}

void Bytecodes::WriteFloat(float f)
{
	Write(reinterpret_cast<const char*>(&f), sizeof(float));
}

void Bytecodes::WriteInt(int i)
{
	Write(reinterpret_cast<const char*>(&i), sizeof(int));
}

void Bytecodes::WriteBool(bool b)
{
	Write(reinterpret_cast<const char*>(&b), sizeof(bool));
}

void Bytecodes::Write(const char* data, size_t size)
{
	if (m_curr_pos < 0) 
	{
		std::copy(data, data + size, std::back_inserter(m_code));
	} 
	else if (m_curr_pos + size <= m_code.size()) 
	{
		for (int i = 0; i < size; ++i) {
			m_code[m_curr_pos + i] = data[i];
		}
		m_curr_pos += static_cast<int>(size);
	}
}

}