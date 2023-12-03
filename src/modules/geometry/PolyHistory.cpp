#include "PolyHistory.h"

#include <assert.h>

namespace tt
{

void PolyHistory::AddGenerated(const PolyPtr& initial, const PolyPtr& generated)
{
	auto itr = m_add_list.find(initial);
	assert(itr == m_add_list.end());
	m_add_list.insert({ initial, generated });
}

void PolyHistory::AddDeleted(const PolyPtr& poly)
{
	auto itr = m_del_list.find(poly);
	assert(itr == m_del_list.end());
	m_del_list.insert(poly);
}

void PolyHistory::AddModified(const PolyPtr& initial, const PolyPtr& modified)
{
	auto itr = m_mod_list.find(initial);
	assert(itr == m_mod_list.end());
	m_mod_list.insert({ initial, modified });
}

}