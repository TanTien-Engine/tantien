#pragma once

#include <SM_Rect.h>

#include <vector>
#include <memory>

namespace gs { class Shape2D; }

namespace tt
{

class ShapeMaths
{
public:
	static std::vector<std::shared_ptr<gs::Shape2D>> 
		Scissor(const std::shared_ptr<gs::Shape2D>& shape, const sm::rect& rect);

}; // ShapeMaths

}