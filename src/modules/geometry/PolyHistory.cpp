#include "PolyHistory.h"

#include <assert.h>

namespace tt
{

PolyHistory::PolyHistory(const PolyHistory& hist)
{
	m_add_list = hist.m_add_list;
	m_del_list = hist.m_del_list;
	m_mod_list = hist.m_mod_list;
}

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