#include "Bytecodes.h"

namespace tt
{

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
		m_curr_pos += size;
	}
}

}