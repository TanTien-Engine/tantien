#pragma once

#include <easyvm/Value.h>

#include <vector>

namespace tt
{

class ValueCache
{
public:
	void Resize(size_t size);

	bool SetValue(int idx, const evm::Value& val);
	bool GetValue(int idx, evm::Value& val) const;

private:
	std::vector<evm::Value> m_vals;

}; // ValueCache

}