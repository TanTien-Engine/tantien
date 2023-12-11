#include "PolyDiff.h"

#include <assert.h>

namespace tt
{

PolyDiff::PolyDiff(const PolyDiff& diff)
{
	m_add_list = diff.m_add_list;
	m_del_list = diff.m_del_list;
	m_mod_list = diff.m_mod_list;
}

void PolyDiff::AddGenerated(const PolyPtr& initial, const PolyPtr& generated)
{
	m_add_list.push_back({ initial, generated });
}

void PolyDiff::AddDeleted(const PolyPtr& poly)
{
	auto itr = m_del_list.find(poly);
	assert(itr == m_del_list.end());
	m_del_list.insert(poly);
}

void PolyDiff::AddModified(const PolyPtr& initial, const PolyPtr& modified)
{
	auto itr = m_mod_list.find(initial);
	assert(itr == m_mod_list.end());
	m_mod_list.insert({ initial, modified });
}

}