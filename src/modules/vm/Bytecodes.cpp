#include "Bytecodes.h"

#include <assert.h>

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
		m_curr_pos += static_cast<int>(size);
	}
}

void Bytecodes::WriteNum(int pos, float num)
{
	SetCurrPos(pos);
	Write(reinterpret_cast<const char*>(&num), sizeof(float));
}

void Bytecodes::AddOptimBlock(size_t hash, int begin, int end, int reg)
{
	CodeBlock b;
	b.hash = hash;
	b.begin = begin;
	b.end = end;
	b.reg = reg;
	b.times = 1;

	auto itr = m_blocks.find(hash);
	if (itr == m_blocks.end())
	{
		m_blocks.insert({ hash, { b } });
	}
	else
	{
		assert(begin - end == itr->second.front().begin - itr->second.front().end);
		itr->second.push_back(b);
	}
}

void Bytecodes::StatCall(const std::string& name)
{
	auto itr = m_stat_call.find(name);
	if (itr == m_stat_call.end()) {
		m_stat_call.insert({ name, 1 });
	} else {
		++itr->second;
	}
}

}