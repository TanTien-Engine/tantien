#pragma once

#include <memory>

namespace spatialdb { class RTree; }
namespace pm3 { class Polytope; }

namespace tt
{

class BRepKey;
class PolyDiff;

class RTreeUpdate
{
public:
	static std::shared_ptr<tt::BRepKey> Insert(
		const std::shared_ptr<spatialdb::RTree>& rtree, 
		const std::shared_ptr<pm3::Polytope>& poly
	);

	static void Clear(std::shared_ptr<spatialdb::RTree>& rtree);

	static void RollForward(std::shared_ptr<spatialdb::RTree>& rtree,
		const std::shared_ptr<tt::PolyDiff>& diff);
	static void RollBack(std::shared_ptr<spatialdb::RTree>& rtree,
		const std::shared_ptr<tt::PolyDiff>& diff);

}; // RTreeUpdate

}