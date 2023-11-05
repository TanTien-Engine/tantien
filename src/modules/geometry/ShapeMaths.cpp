#include "modules/geometry/ShapeMaths.h"

#include <geoshape/Line2D.h>
#include <geoshape/Polyline2D.h>
#include <geoshape/Polygon2D.h>
#include <geoshape/Rect.h>
#include <SM_Calc.h>
#include <SM_Polyline.h>
#include <SM_DouglasPeucker.h>

// fixme
#include "../../../../littleworld/citygen/Extrude.h"

#include <iterator>
#include <set>

namespace
{

const float POLYLINE_SIMPLIFY_PRECISION = 2.5f;

bool is_pos_outside(const sm::vec2& pos, const sm::rect& rect)
{
	return pos.x < rect.xmin || pos.x > rect.xmax
		|| pos.y < rect.ymin || pos.y > rect.ymax;
}

bool is_sampe_pos(const sm::vec2& p0, const sm::vec2& p1)
{
	auto d = sm::dis_pos_to_pos(p0, p1);
	return d < POLYLINE_SIMPLIFY_PRECISION;
}

bool is_same_polyline(const std::vector<sm::vec2>& p0, const std::vector<sm::vec2>& p1)
{
	if (p0.size() != p1.size()) {
		return false;
	}

	if (p0 == p1) {
		return true;
	}
	
	for (size_t i = 0, n = p0.size(); i < n; ++i) {
		auto d = sm::dis_pos_to_pos(p0[i], p1[i]);
		if (d > SM_LARGE_EPSILON) {
			return false;
		}
	}

	return true;
}

std::vector<sm::vec2> clean_polyline(const std::vector<sm::vec2>& verts)
{
	std::vector<sm::vec2> ret;

	if (verts.empty()) {
		return ret;
	}

	ret.push_back(verts.front());
	for (size_t i = 1, n = verts.size(); i < n; ++i) 
	{
		auto d = sm::dis_pos_to_pos(ret.back(), verts[i]);
		if (d > 0.1f) {
			ret.push_back(verts[i]);
		}
	}

	return ret;
}

std::vector<sm::vec2> prepare_polygon_loop(const std::vector<sm::vec2>& verts, bool is_border)
{
	auto fixed = clean_polyline(verts);
	if (sm::is_polygon_clockwise(fixed) == is_border) {
		std::reverse(fixed.begin(), fixed.end());
	}
	return fixed;
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

std::shared_ptr<gs::Shape2D>
ShapeMaths::Expand(const std::shared_ptr<gs::Shape2D>& shape, float dist)
{
	std::vector<std::vector<sm::vec2>> loops;
	switch (shape->GetType())
	{
	case gs::ShapeType2D::Line:
	{
		auto line = std::static_pointer_cast<gs::Line2D>(shape);
		auto& s = line->GetStart();
		auto& e = line->GetEnd();
		if (is_sampe_pos(s, e)) {
			return nullptr;
		}
		auto vertices = { s, e };
		loops = sm::polyline_expand(vertices, dist, false);
	}
		break;
	case gs::ShapeType2D::Polyline:
	{
		auto polyline = std::static_pointer_cast<gs::Polyline2D>(shape);
		auto& verts = polyline->GetVertices();
		auto fixed = clean_polyline(verts);

		if (fixed.size() < 2) {
			return nullptr;
		}

		if (is_sampe_pos(fixed.front(), fixed.back()))
		{
			if (fixed.size() <= 2) {
				return nullptr;
			}

			auto border = fixed;
			border.pop_back();
			loops = sm::polyline_expand(border, dist, true);
		}
		else
		{
			loops = sm::polyline_expand(fixed, dist, false);
		}
		assert(!loops.empty());
	}
		break;
	case gs::ShapeType2D::Rect:
	{
		auto rect = std::static_pointer_cast<gs::Rect>(shape);
		auto& r = rect->GetRect();
		auto border = {
			sm::vec2(r.xmin, r.ymin),
			sm::vec2(r.xmax, r.ymin),
			sm::vec2(r.xmax, r.ymax),
			sm::vec2(r.xmin, r.ymax),
		};
		loops = sm::polyline_expand(border, dist, true);
	}
		break;
	}

	if (loops.empty()) {
		return nullptr;
	}

	auto poly = std::make_shared<gs::Polygon2D>();
	poly->SetVertices(prepare_polygon_loop(loops[0], true));
	for (size_t i = 1, n = loops.size(); i < n; ++i) {
		poly->AddHole(prepare_polygon_loop(loops[i], false));
	}

	return poly;
}

std::shared_ptr<pm3::Polytope>
ShapeMaths::Extrude(const std::shared_ptr<gs::Shape2D>& shape, float dist)
{
	std::shared_ptr<pm3::Polytope> ret = nullptr;

	switch (shape->GetType())
	{
	case gs::ShapeType2D::Polygon:
	{
		auto poly = std::static_pointer_cast<gs::Polygon2D>(shape);

		auto verts = poly->GetVertices();
		for (auto& v : verts) {
			v *= 0.01f;
		}
		verts = sm::douglas_peucker(verts, SM_LARGE_EPSILON);
		poly->SetVertices(verts);

		auto holes = poly->GetHoles();
		for (auto& hole : holes) 
		{
			for (auto& v : hole) {
				v *= 0.01f;
			}
			hole = sm::douglas_peucker(hole, SM_LARGE_EPSILON);
		}
		poly->SetHoles(holes);

		ret = citygen::Extrude::Face(poly, dist);
	}
		break;
	}

	return ret;
}

std::vector<std::shared_ptr<gs::Shape2D>>
ShapeMaths::Merge(const std::vector<std::shared_ptr<gs::Shape2D>>& shapes)
{
	std::map<sm::vec2, std::vector<std::shared_ptr<gs::Shape2D>>> pos2shapes;

	auto map_find = [&](const sm::vec2& pos)
	{
		auto itr = pos2shapes.find(pos);
		if (itr != pos2shapes.end()) {
			return itr;
		}

		for (auto i = pos2shapes.begin(); i != pos2shapes.end(); ++i) {
			if (is_sampe_pos(pos, i->first)) {
				return i;
			}
		}

		return pos2shapes.end();
	};

	auto map_insert = [&](const sm::vec2& pos, const std::shared_ptr<gs::Shape2D>& shape)
	{
		auto itr = map_find(pos);
		if (itr == pos2shapes.end()) 
		{
			pos2shapes[pos] = { shape };
		} 
		else 
		{
			bool exist = false;
			for (auto old : itr->second) {
				if (old == shape) {
					exist = true;
				}
			}
			if (!exist) {
				itr->second.push_back(shape);
			}
		}
	};

	auto map_remove = [&](const sm::vec2& pos, const std::shared_ptr<gs::Shape2D>& shape)
	{
		auto itr = map_find(pos);
		if (itr == pos2shapes.end()) {
			return;
		}

		for (auto i = itr->second.begin(); i != itr->second.end(); ) 
		{
			if (*i == shape) {
				i = itr->second.erase(i);
			} else {
				++i;
			}
		}
		if (itr->second.empty()) {
			pos2shapes.erase(itr);
		}
	};

	auto get_verts = [](const std::shared_ptr<gs::Shape2D>& shape) -> std::vector<sm::vec2>
	{
		std::vector<sm::vec2> verts;

		auto type = shape->GetType();
		if (type == gs::ShapeType2D::Line) {
			auto line = std::static_pointer_cast<gs::Line2D>(shape);
			verts = { line->GetStart(), line->GetEnd() };
		} else {
			auto polyline = std::static_pointer_cast<gs::Polyline2D>(shape);
			verts = polyline->GetVertices();
		}

		return verts;
	};

	std::vector<std::shared_ptr<gs::Shape2D>> ret;

	for (auto& shape : shapes)
	{
		auto type = shape->GetType();
		if (type != gs::ShapeType2D::Line &&
			type != gs::ShapeType2D::Polyline)
		{
			ret.push_back(shape);
			continue;
		}

		sm::vec2 s, e;
		if (type == gs::ShapeType2D::Line) 
		{
			auto line = std::static_pointer_cast<gs::Line2D>(shape);
			s = line->GetStart();
			e = line->GetEnd();
		} 
		else if (type == gs::ShapeType2D::Polyline) 
		{
			auto polyline = std::static_pointer_cast<gs::Polyline2D>(shape);
			auto& verts = polyline->GetVertices();
			if (verts.size() < 2) {
				ret.push_back(shape);
				continue;
			}
			s = verts.front();
			e = verts.back();
		}

		auto itr_s = map_find(s);
		auto itr_e = map_find(e);
		if (itr_s == pos2shapes.end() && itr_e == pos2shapes.end())
		{
			pos2shapes[s] = { shape };
			pos2shapes[e] = { shape };
		}
		else
		{
			std::shared_ptr<gs::Shape2D> old_shape = nullptr;
			if (itr_s != pos2shapes.end())
			{
				auto& list = itr_s->second;
				assert(!list.empty());
				old_shape = list.back();
			}
			else
			{
				auto& list = itr_e->second;
				assert(!list.empty());
				old_shape = list.back();
			}
			assert(old_shape);

			auto old_verts = get_verts(old_shape);
			auto new_verts = get_verts(shape);

			bool is_part_of_old = false;
			if (new_verts.size() <= old_verts.size())
			{
				std::vector<sm::vec2> part;
				std::copy(old_verts.begin(), old_verts.begin() + new_verts.size(), std::back_inserter(part));
				if (is_same_polyline(part, new_verts)) {
					is_part_of_old = true;
				} else {
					std::reverse(part.begin(), part.end());
					if (is_same_polyline(part, new_verts)) {
						is_part_of_old = true;
					} else {
						part.clear();
						std::copy(old_verts.rbegin(), old_verts.rbegin() + new_verts.size(), std::back_inserter(part));
						if (is_same_polyline(part, new_verts)) {
							is_part_of_old = true;
						} else {
							std::reverse(part.begin(), part.end());
							if (is_same_polyline(part, new_verts)) {
								is_part_of_old = true;
							}
						}
					}
				}
			}
			if (is_part_of_old) {
				continue;
			}

			map_remove(old_verts.front(), old_shape);
			map_remove(old_verts.back(), old_shape);

			std::vector<sm::vec2> merged;
			bool s_changed = false;
			if (is_sampe_pos(old_verts.front(), new_verts.front()))
			{
				std::copy(new_verts.rbegin(), new_verts.rend(), std::back_inserter(merged));
				merged.pop_back();
				std::copy(old_verts.begin(), old_verts.end(), std::back_inserter(merged));
				s_changed = true;
			} 
			else if (is_sampe_pos(old_verts.front(), new_verts.back()))
			{
				std::copy(new_verts.begin(), new_verts.end(), std::back_inserter(merged));
				merged.pop_back();
				std::copy(old_verts.begin(), old_verts.end(), std::back_inserter(merged));
				s_changed = true;
			} 
			else if (is_sampe_pos(old_verts.back(), new_verts.front()))
			{
				std::copy(old_verts.begin(), old_verts.end(), std::back_inserter(merged));
				merged.pop_back();
				std::copy(new_verts.begin(), new_verts.end(), std::back_inserter(merged));
				s_changed = false;
			} 
			else if (is_sampe_pos(old_verts.back(), new_verts.back()))
			{
				std::copy(old_verts.begin(), old_verts.end(), std::back_inserter(merged));
				merged.pop_back();
				std::copy(new_verts.rbegin(), new_verts.rend(), std::back_inserter(merged));
				s_changed = false;
			}

			auto fixed = clean_polyline(merged);
			if (fixed.size() > 1)
			{
				auto new_shape = std::make_shared<gs::Polyline2D>(fixed, false);
				map_insert(fixed.front(), new_shape);
				map_insert(fixed.back(), new_shape);
			}
		}
	}

	std::set<std::shared_ptr<gs::Shape2D>> unique_set;
	for (auto itr : pos2shapes) {
		for (auto s : itr.second) {
			unique_set.insert(s);
		}
	}
	for (auto s : unique_set) {
		ret.push_back(s);
	}

	return ret;
}

}