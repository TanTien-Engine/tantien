#pragma once

#include <brepdb/Region.h>

namespace tt
{

class BRepKey
{
public:
	brepdb::Region r;
	int64_t id = -1;

}; // BRepKey

}