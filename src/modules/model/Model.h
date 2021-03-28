#pragma once

#include "core/macro.h"

#include <unirender/VertexLayoutType.h>
#include <unirender/PrimitiveType.h>

#include <memory>

namespace ur { class Device; class VertexArray; }

namespace tt
{

enum class ShapeType
{
	Quad = 0,
	Cube,
	Sphere,
	Grids,
};

class Model
{
public:
	std::shared_ptr<ur::VertexArray> CreateShape(const ur::Device& dev, ShapeType type, 
		ur::VertexLayoutType layout, ur::PrimitiveType& prim_type);

	std::shared_ptr<ur::VertexArray> CreateGrids(const ur::Device& dev,
		ur::VertexLayoutType layout, size_t size);

private:

	TT_SINGLETON_DECLARATION(Model)

}; // Model

}