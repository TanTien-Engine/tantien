#pragma once

#include <vector>
#include <set>
#include <map>
#include <memory>

namespace pm3 { class Polytope; }

using PolyPtr = std::shared_ptr<pm3::Polytope>;

namespace tt
{

class PolyDiff
{
public:
	PolyDiff() {}
	PolyDiff(const PolyDiff& diff);

	void AddGenerated(const PolyPtr& initial, const PolyPtr& generated);
	void AddDeleted(const PolyPtr& poly);
	void AddModified(const PolyPtr& initial, const PolyPtr& modified);

	auto& GetAddList() const { return m_add_list; }
	auto& GetDelList() const { return m_del_list; }
	auto& GetModList() const { return m_mod_list; }

private:
	std::vector<std::pair<PolyPtr, PolyPtr>> m_add_list;
	std::set<PolyPtr> m_del_list;
	std::map<PolyPtr, PolyPtr> m_mod_list;

}; // PolyDiff

}