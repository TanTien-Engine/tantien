#pragma once

#include <easyvm/Value.h>

#include <vector>

namespace tt
{

class ValueCache
{
public:
	void Resize(size_t size);

	void SetValue(int idx, const evm::Value& val);
	const evm::Value& GetValue(int idx) const;

private:
	std::vector<evm::Value> m_vals;

}; // ValueCache

}