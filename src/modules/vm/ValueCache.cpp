#include "ValueCache.h"

namespace tt
{

void ValueCache::Resize(size_t size)
{
	m_vals.resize(size);
}

bool ValueCache::SetValue(int idx, const evm::Value& val)
{
	if (idx >= 0 && idx < m_vals.size()) {
		m_vals[idx] = val;
		return true;
	} else {
		return false;
	}
}

bool ValueCache::GetValue(int idx, evm::Value& val) const
{
	if (idx < 0 || idx >= m_vals.size())
	{
		return false;
	}
	else
	{
		val = m_vals[idx];
		return true;
	}
}

}