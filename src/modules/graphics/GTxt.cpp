#include "modules/graphics/GTxt.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/DTex.h"
#include "modules/graphics/LoadingList.h"

#include <unirender/typedef.h>
#include <unirender/Texture.h>
#include <tessellation/Painter.h>
#include <gtxt_richtext.h>
#include <gtxt_freetype.h>

#include <array>

namespace
{

const ur::Device* UR_DEV = nullptr;

/************************************************************************/
/* render                                                               */
/************************************************************************/

struct render_params
{
	const sm::Matrix2D* mt  = nullptr;
	uint32_t mul = 0xffffffff;
	uint32_t add = 0;
	tess::Painter*    pt  = nullptr;
	bool texcoords_relocate = true;
    ur::Context*     ctx = nullptr;
};

void
render_glyph(const ur::TexturePtr& tex, const float* _texcoords, float x, float y, float w, float h, const gtxt_draw_style* ds, render_params* rp)
{
	x += ds->offset_x;
	y += ds->offset_y;
	float hw = w * 0.5f * ds->scale, hh = h * 0.5f * ds->scale;

	std::array<sm::vec2, 4> vertices;
	vertices[0] = sm::vec2(x - hw, y - hh);
	vertices[1] = sm::vec2(x + hw, y - hh);
	vertices[2] = sm::vec2(x + hw, y + hh);
	vertices[3] = sm::vec2(x - hw, y + hh);
	for (int i = 0; i < 4; ++i) {
		vertices[i] = *rp->mt * vertices[i];
	}

	std::array<sm::vec2, 4> texcoords;
	texcoords[0].Set(_texcoords[0], _texcoords[1]);
	texcoords[1].Set(_texcoords[2], _texcoords[3]);
	texcoords[2].Set(_texcoords[4], _texcoords[5]);
	texcoords[3].Set(_texcoords[6], _texcoords[7]);

	// todo: gray text with filter shader
	if (rp->pt) {
		rp->pt->AddTexQuad(tex->GetTexID(), vertices, texcoords, 0xffffffff);
	} else {
		tt::Graphics::Instance()->DrawTexQuad(&vertices[0].x, &texcoords[0].x, tex, 0xffffffff);
	}
}

void
render_decoration(const sm::Matrix2D& mat, float x, float y, float w, float h, const gtxt_draw_style* ds, render_params* rp)
{
	const gtxt_decoration* d = &ds->decoration;
	if (d->type == GRDT_NULL) {
		return;
	}

	tess::Painter pt;

	float hw = w * 0.5f,
		  hh = h * 0.5f;
	const uint32_t col = 0xffffffff;
	if (d->type == GRDT_OVERLINE || d->type == GRDT_UNDERLINE || d->type == GRDT_STRIKETHROUGH) {
		sm::vec2 left(x - hw, y), right(x + hw, y);
		switch (d->type)
		{
		case GRDT_OVERLINE:
			left.y = right.y = ds->row_y + ds->row_h;
			break;
		case GRDT_UNDERLINE:
			left.y = right.y = ds->row_y;
			break;
		case GRDT_STRIKETHROUGH:
			left.y = right.y = ds->row_y + ds->row_h * 0.5f;
			break;
		}
		pt.AddLine(mat * left, mat * right, col);
	} else if (d->type == GRDT_BORDER || d->type == GRDT_BG) {
		sm::vec2 min(x - hw, ds->row_y),
			     max(x + hw, ds->row_y + ds->row_h);
		min = mat * min;
		max = mat * max;
		if (d->type == GRDT_BG) {
			pt.AddRectFilled(min, max, col);
		} else if (ds->pos_type != GRPT_NULL) {
			pt.AddLine(min, sm::vec2(max.x, min.y), col);
			pt.AddLine(sm::vec2(min.x, max.y), max, col);
			if (ds->pos_type == GRPT_BEGIN) {
				pt.AddLine(min, sm::vec2(min.x, max.y), col);
			}
			if (ds->pos_type == GRPT_END) {
				pt.AddLine(sm::vec2(max.x, min.y), max, col);
			}
		}
	}

	tt::Graphics::Instance()->DrawPainter(pt);
}

void
render(const ur::TexturePtr& tex, const float* texcoords, float x, float y, float w, float h, const gtxt_draw_style* ds, void* ud)
{
	render_params* rp = (render_params*)ud;
	if (ds) {
		if (ds->decoration.type == GRDT_BG) {
			render_decoration(*rp->mt, x, y, w, h, ds, rp);
			render_glyph(tex, texcoords, x, y, w, h, ds, rp);
		} else {
			render_glyph(tex, texcoords, x, y, w, h, ds, rp);
			render_decoration(*rp->mt, x, y, w, h, ds, rp);
		}
	} else {
		struct gtxt_draw_style ds;
		ds.alpha = 1;
		ds.scale = 1;
		ds.offset_x = ds.offset_y = 0;
		render_glyph(tex, texcoords, x, y, w, h, &ds, rp);
	}
}

bool
has_rotated_gradient_color(const gtxt_glyph_color& col)
{
	return (col.mode_type == 1 && fabs(col.mode.TWO.angle) > FLT_EPSILON)
		|| (col.mode_type == 2 && fabs(col.mode.THREE.angle) > FLT_EPSILON);
}

int NEXT_GLPHY_ID = 0;

static const int GLPHY_HASH_CAP = 197;
std::vector<std::pair<gtxt_glyph_style, int>> GLYPH_HASH[GLPHY_HASH_CAP];

std::pair<gtxt_glyph_style, int> GLYPH_LAST;

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

void hash_glyph_color(size_t& seed, const gtxt_glyph_color& color)
{
	// todo multi colors
	hash_combine(seed, color.mode.ONE.color.integer);
}

size_t hash_glyph_style(const gtxt_glyph_style& gs, float line_x)
{
	size_t hash = 0;
	hash_combine(hash, gs.font);
	hash_combine(hash, gs.font_size);
	hash_combine(hash, gs.font);
	hash_glyph_color(hash, gs.font_color);
	hash_combine(hash, gs.edge);
	if (gs.edge)
	{
		hash_combine(hash, gs.edge_size);
		hash_glyph_color(hash, gs.edge_color);
	}
	hash_combine(hash, line_x);
	return hash;
}

bool glyph_style_eq(const gtxt_glyph_style& s0, const gtxt_glyph_style& s1)
{
	if (s0.font != s1.font || s0.font_size != s1.font_size ||
		s0.font_color.mode.ONE.color.integer != s1.font_color.mode.ONE.color.integer ||
		s0.edge != s1.edge) {
		return false;
	}

	if (s0.edge)
	{
		if (s0.edge_size != s1.edge_size ||
			s0.edge_color.mode.ONE.color.integer != s1.edge_color.mode.ONE.color.integer) {
			return false;
		}
	}

	return true;
}

int gen_glyph_style_id(const gtxt_glyph_style& style, float line_x)
{
	if (glyph_style_eq(style, GLYPH_LAST.first)) {
		return GLYPH_LAST.second;
	}

	int hash = hash_glyph_style(style, line_x) % GLPHY_HASH_CAP;
	auto& list = GLYPH_HASH[hash];
	for (int i = 0, n = list.size(); i < n; ++i) {
		if (glyph_style_eq(list[i].first, style)) {
			GLYPH_LAST = list[i];
			return list[i].second;
		}
	}

	int id = NEXT_GLPHY_ID++;

	list.push_back(std::make_pair(style, id));

	GLYPH_LAST.first = style;
	GLYPH_LAST.second = id;

	return id;
}

tt::UID calc_glyph_uid(int unicode, const gtxt_glyph_style& style, float line_x)
{
	uint64_t style_id = gen_glyph_style_id(style, line_x);
	uint64_t id = ((style_id & 0xffffffff) << 32) | unicode;
	return tt::ResourceUID::Compose(id, tt::ResourceUID::KeyType::GLYPH);
}

void
draw_glyph(int unicode, float x, float y, float w, float h, float start_x,
		   const gtxt_glyph_style* gs, const gtxt_draw_style* ds, void* ud)
{
	float line_x = 0;
	if (has_rotated_gradient_color(gs->font_color) ||
		gs->edge && has_rotated_gradient_color(gs->edge_color)) {
		line_x = x - start_x;
	}
	uint64_t uid = calc_glyph_uid(unicode, *gs, line_x);

	auto dtex = tt::DTex::Instance();

	render_params* rp = (render_params*)ud;
	if (rp->texcoords_relocate)
	{
        ur::TexturePtr texture = nullptr;
		int block_id;
		int ft_count = gtxt_ft_get_font_cout();

		const float* texcoords = nullptr;
		bool exist = false;
		if (gs->font < ft_count && !dtex->ExistGlyph(uid)) {
			exist = false;
			texcoords = nullptr;
		} else {
			exist = true;
			texcoords = dtex->QuerySymbol(uid, texture, block_id);
		}

		if (texcoords)
		{
			render(texture, texcoords, x, y, w, h, ds, ud);
		}
		else
		{
			if (gs->font < ft_count)
			{
				float texcoords[8];
				if (exist && dtex->QueryGlyph(uid, texcoords, texture))
				{
					render(texture, texcoords, x, y, w, h, ds, ud);
				}
				else
				{
					tt::LoadingList::Instance()->AddGlyph(uid, unicode, line_x, *gs);
				}
			}
			else
			{
				int uf_font = gs->font - ft_count;
	//			dtex->DrawUFChar(unicode, uf_font, x, y, ud);
			}
		}
	}
	else
	{
		float texcoords[8];
		ur::TexturePtr tex;
		if (dtex->QueryGlyph(uid, texcoords, tex)) {
			render(tex, texcoords, x, y, w, h, ds, ud);
		} else {
			tt::LoadingList::Instance()->AddGlyph(uid, unicode, line_x, *gs);
		}
	}
}

/************************************************************************/
/* richtext extern symbol                                               */
/************************************************************************/

void*
ext_sym_create(const char* str) {
	return nullptr;
}

void
ext_sym_release(void* ext_sym) {
}

void
ext_sym_get_size(void* ext_sym, int* width, int* height) {
}

void
ext_sym_render(void* ext_sym, float x, float y, void* ud) {
}

/************************************************************************/
/* user font                                                            */
/************************************************************************/

void
get_uf_layout(int unicode, int font, struct gtxt_glyph_layout* layout) {
//	facade::GTxt::Instance()->GetUFLayout(unicode, font, layout);
}

}

namespace tt
{

TT_SINGLETON_DEFINITION(GTxt)

int GTxt::m_cap_bitmap = 50;
int GTxt::m_cap_layout = 500;

GTxt::GTxt()
{
}

void GTxt::Init(const ur::Device& dev)
{
    UR_DEV = &dev;

    gtxt_label_cb_init(draw_glyph);

    gtxt_ft_create();

    gtxt_glyph_create(m_cap_bitmap, m_cap_layout, nullptr, get_uf_layout);

    gtxt_richtext_ext_sym_cb_init(&ext_sym_create, &ext_sym_release, &ext_sym_get_size, &ext_sym_render, nullptr);
}

void GTxt::Draw(ur::Context& ctx, const std::string& text, const gtxt_label_style& style, const sm::Matrix2D& mat,
	            uint32_t mul, uint32_t add, int time, bool richtext, tess::Painter* pt, bool texcoords_relocate)
{
	render_params rp;
	rp.mt  = &mat;
	rp.mul = mul;
	rp.add = add;
	rp.pt  = pt;
	rp.texcoords_relocate = texcoords_relocate;
    rp.ctx = &ctx;

	if (richtext) {
		gtxt_label_draw_richtext(text.c_str(), &style, time, (void*)&rp);
	} else {
		gtxt_label_draw(text.c_str(), &style, (void*)&rp);
	}
}

//sm::vec2 GTxt::CalcLabelSize(const std::string& text, const pt2::Textbox& style)
//{
//	gtxt_label_style gtxt_style;
//	LoadLabelStyle(gtxt_style, style);
//
//	sm::vec2 sz;
//	gtxt_get_label_size(text.c_str(), &gtxt_style, &sz.x, &sz.y);
//
//	return sz;
//}

void GTxt::LoadFonts(const std::vector<std::pair<std::string, std::string>>& fonts,
	                 const std::vector<std::pair<std::string, std::string>>& user_fonts)
{
	for (auto& pair : fonts) {
		LoadFont(pair.first, pair.second);
	}
	for (auto& pair : user_fonts) {
		LoadUserFont(pair.first, pair.second);
	}
}

void GTxt::LoadFont(const std::string& name, const std::string& filepath)
{
	auto itr = m_fonts.find(name);
	if (itr != m_fonts.end()) {
		return;
	}

	gtxt_ft_add_font(name.c_str(), filepath.c_str());
	m_fonts.insert(name);
}

void GTxt::LoadUserFont(const std::string& name, const std::string& filepath)
{
	// todo
}

void GTxt::LoadUserFontChar(const std::string& str, const std::string& pkg, const std::string& node)
{
	// todo
}

}