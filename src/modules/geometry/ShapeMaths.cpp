#include "modules/geometry/ShapeMaths.h"

#include <geoshape/Line2D.h>
#include <geoshape/Polyline2D.h>
#include <geoshape/Polygon2D.h>
#include <SM_Calc.h>

namespace
{

bool is_pos_outside(const sm::vec2& pos, const sm::rect& rect)
{
	return pos.x < rect.xmin || pos.x > rect.xmax
		|| pos.y < rect.ymin || pos.y > rect.ymax;
}

}

namespace tt
{

std::vector<std::shared_ptr<gs::Shape2D>> 
ShapeMaths::Scissor(const std::shared_ptr<gs::Shape2D>& shape, const sm::rect& rect)
{
	std::vector<std::shared_ptr<gs::Shape2D>> ret;

	switch (shape->GetType())
	{
	case gs::ShapeType2D::Line:
	{
		auto line = std::static_pointer_cast<gs::Line2D>(shape);
		auto& s = line->GetStart();
		auto& e = line->GetEnd();

		std::vector<sm::vec2> cross;
		sm::intersect_segment_rect(s, e, rect, cross);
		switch (cross.size())
		{
		case 0:
			if (!is_pos_outside(s, rect) ||
				!is_pos_outside(e, rect)) {
				ret.push_back(shape);
			}
			break;
		case 1:
		{
			if (!is_pos_outside(s, rect))
			{
				auto new_line = std::make_shared<gs::Line2D>(cross[0], s);
				ret.push_back(new_line);
			}
			else if (!is_pos_outside(e, rect))
			{
				auto new_line = std::make_shared<gs::Line2D>(cross[0], e);
				ret.push_back(new_line);
			}
		}
			break;
		case 2:
		{
			auto new_line = std::make_shared<gs::Line2D>(cross[0], cross[1]);
			ret.push_back(new_line);
		}
			break;
		}
	}
		break;
	case gs::ShapeType2D::Polyline:
	{
		auto polyline = std::static_pointer_cast<gs::Polyline2D>(shape);
		auto& verts = polyline->GetVertices();
		if (verts.size() < 2) {
			return ret;
		}

		std::vector<sm::vec2> new_verts;
		for (size_t i = 0, n = verts.size(); i < n - 1; ++i)
		{
			auto& s = verts[i];
			auto& e = verts[i + 1];

			bool s_in = !is_pos_outside(s, rect);
			bool e_in = !is_pos_outside(e, rect);
			if (s_in && e_in)
			{
				new_verts.push_back(s);
			}
			else
			{
				if (s_in) {
					new_verts.push_back(s);
				}

				std::vector<sm::vec2> cross;
				sm::intersect_segment_rect(s, e, rect, cross);
				switch (cross.size())
				{
				case 0:
					if (s_in || e_in)
					{
						new_verts.push_back(e);
					} 
					else if (!new_verts.empty()) 
					{
						if (new_verts.size() > 1) {
							ret.push_back(std::make_shared<gs::Polyline2D>(new_verts, false));
						}
						new_verts.clear();
					}
					break;
				case 1:
				{
					new_verts.push_back(cross[0]);
				}
					break;
				case 2:
				{
					new_verts.push_back(cross[0]);
					new_verts.push_back(cross[1]);
				}
					break;
				}
			}
		}

		if (!is_pos_outside(verts.back(), rect)) {
			new_verts.push_back(verts.back());
		}
		if (new_verts.size() > 1) {
			ret.push_back(std::make_shared<gs::Polyline2D>(new_verts, false));
		}
	}
		break;
	}

	return ret;
}

std::vector<std::shared_ptr<gs::Shape2D>>
ShapeMaths::Expand(const std::shared_ptr<gs::Shape2D>& shape, float dist)
{
	std::vector<std::vector<sm::vec2>> loops;
	switch (shape->GetType())
	{
	case gs::ShapeType2D::Line:
	{
		auto line = std::static_pointer_cast<gs::Line2D>(shape);
		auto vertices = { line->GetStart(), line->GetEnd() };
		loops = sm::polyline_expand(vertices, dist, false);
	}
		break;
	case gs::ShapeType2D::Polyline:
	{
		auto polyline = std::static_pointer_cast<gs::Polyline2D>(shape);
		auto& vertices = polyline->GetVertices();
		bool is_closed = vertices.size() > 1 && vertices.front() == vertices.back();
		loops = sm::polyline_expand(polyline->GetVertices(), dist, is_closed);
	}
		break;
	}

	std::vector<std::shared_ptr<gs::Shape2D>> ret;
	for (auto& loop : loops) 
	{
		auto fixed = sm::douglas_peucker(loop, POLYLINE_SIMPLIFY_PRECISION);
		if (fixed.size() <= 2) {
			continue;
		}

		auto poly = std::make_shared<gs::Polygon2D>();
		poly->SetVertices(fixed);

		ret.push_back(poly);
	}

	return ret;
}

}