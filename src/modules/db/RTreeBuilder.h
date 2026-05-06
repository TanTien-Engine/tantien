#pragma once

namespace spatialdb { class RTree; }

namespace tt
{

class RTreeBuilder
{
public:
	static void FromModeling(spatialdb::RTree& rtree, const char* filepath);

}; // RTreeBuilder

}