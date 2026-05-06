#pragma once

#include <spatialdb/Region.h>

namespace tt
{

class BRepKey
{
public:
	spatialdb::Region r;
	int64_t id = -1;

}; // BRepKey

}