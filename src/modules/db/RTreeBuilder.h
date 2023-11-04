#pragma once

namespace brepdb { class RTree; }

namespace tt
{

class RTreeBuilder
{
public:
	static void FromModeling(brepdb::RTree& rtree, const char* filepath);

}; // RTreeBuilder

}