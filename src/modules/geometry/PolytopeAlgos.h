#pragma once

#include <halfedge/Polygon.h>
#include <polymesh3/Polytope.h>

namespace tt
{

class PolytopeAlgos
{
public:
	static std::vector<pm3::PolytopePtr> 
		Intersect(const std::vector<pm3::PolytopePtr>& a, 
			const std::vector<pm3::PolytopePtr>& b);

	static std::vector<pm3::PolytopePtr>
		Subtract(const std::vector<pm3::PolytopePtr>& a,
			const std::vector<pm3::PolytopePtr>& b);

	static std::vector<pm3::PolytopePtr>
		Union(const std::vector<pm3::PolytopePtr>& a,
			const std::vector<pm3::PolytopePtr>& b);

	static void Extrude(pm3::PolytopePtr& poly, float dist);

	static pm3::PolytopePtr Offset(const pm3::PolytopePtr& poly,
		he::Polygon::KeepType keep, float dist);

private:
	static std::vector<pm3::PolytopePtr>
		SubtractImpl(const std::vector<pm3::PolytopePtr>& a,
			const std::vector<pm3::PolytopePtr>& b);

}; // PolytopeAlgos

}