#include "ValueCache.h"

#include <stdexcept>

namespace tt
{

void ValueCache::Resize(size_t size)
{
	m_vals.resize(size);
}

void ValueCache::SetValue(int idx, const evm::Value& val)
{
    if (idx < 0 || idx >= m_vals.size()) {
        throw std::runtime_error("Error idx!");
    } else {
        m_vals[idx] = val;
    }
}

const evm::Value& ValueCache::GetValue(int idx) const
{
    if (idx < 0 || idx >= m_vals.size()) {
        throw std::runtime_error("Error idx!");
    } else {
        return m_vals[idx];
    }
}

}