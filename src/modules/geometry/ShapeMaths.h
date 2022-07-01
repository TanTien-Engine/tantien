#pragma once

#include <SM_Rect.h>

#include <vector>
#include <memory>

namespace gs { class Shape2D; }
namespace pm3 { class Polytope; }

namespace tt
{

class ShapeMaths
{
public:
	static std::vector<std::shared_ptr<gs::Shape2D>> 
		Scissor(const std::shared_ptr<gs::Shape2D>& shape, const sm::rect& rect);

	static std::vector<std::shared_ptr<gs::Shape2D>>
		Expand(const std::shared_ptr<gs::Shape2D>& shape, float dist);
	static std::shared_ptr<pm3::Polytope>
		Extrude(const std::shared_ptr<gs::Shape2D>& shape, float dist);

}; // ShapeMaths

}