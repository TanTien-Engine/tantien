#include "modules/gui/GUI.h"
#include "modules/graphics/GTxt.h"
#include "modules/graphics/DTex.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/SpriteRenderer.h"
#include "modules/graphics/LoadingList.h"
#include "modules/filesystem/ResourceUID.h"

#include <unirender/Texture.h>
#include <easygui/Callback.h>
#include <tessellation/Painter.h>
#include <tessellation/Palette.h>
#include <gtxt_label.h>

namespace
{

enum TexType
{
	PALETTE,
	LABEL,
	TEX_COUNT,
};
struct Texture
{
	std::shared_ptr<ur::Texture> tex = nullptr;
};
std::array<Texture, TEX_COUNT> TEXTURES;

}

namespace tt
{

TT_SINGLETON_DEFINITION(GUI)

GUI::GUI()
{
	egui::style_colors_dark(ctx.style);

	auto sr = Graphics::Instance()->GetSpriteRenderer();
	TEXTURES[TexType::PALETTE].tex = sr->GetPalette()->GetTexture();
	TEXTURES[TexType::LABEL].tex = DTex::Instance()->GetGlyphFirstPageTex();

	ctx.SetPalette(sr->GetPalette());

	gtxt_label_style st;
	tt::GTxt::InitLabelStype(st);

	st.width = 300;
	st.gs.font_size = ctx.style.font_sz;
	st.align_v = VA_CENTER;
	egui::Callback::Funs cb;
	cb.get_label_sz = [st](const char* label)->sm::vec2 {
		return GTxt::Instance()->CalcLabelSize(label, st);
	};
	cb.draw_label = [st](ur::Context& ctx, const char* label, const sm::vec2& pos, float angle, uint32_t color, tess::Painter& pt)
	{
		if (!label) {
			return;
		}

		sm::Matrix2D mat;
		mat.Rotate(angle);
		const float len = st.width * 0.5f;
		float dx = len * cos(angle);
		float dy = len * sin(angle);
		mat.Translate(pos.x + dx, pos.y + dy);

		GTxt::Instance()->Draw(ctx, label, st, mat, 0xffffffff, 0, 0, false, &pt, true);
	};
	cb.relocate_texcoords = [](tess::Painter& pt)
	{
		auto& palette = TEXTURES[TexType::PALETTE];
		UID uid = ResourceUID::TexQuad(palette.tex->GetTexID(), 0, 0, palette.tex->GetWidth(), palette.tex->GetHeight());
		int block_id;
		ur::TexturePtr cached_tex = nullptr;
		auto cached_texcoords = DTex::Instance()->QuerySymbol(uid, cached_tex, block_id);
		if (!cached_texcoords) {
			LoadingList::Instance()->AddSymbol(uid, palette.tex, sm::irect(sm::ivec2(0, 0), sm::ivec2(palette.tex->GetWidth(), palette.tex->GetHeight())));
			return;
		}

		sm::rect r;
		r.xmin = cached_texcoords[0];
		r.ymin = cached_texcoords[1];
		r.xmax = cached_texcoords[2];
		r.ymax = cached_texcoords[5];
		tt::Graphics::Instance()->GetSpriteRenderer()->GetPalette()->RelocateUV(r);

		//auto relocate_palette = [](const float* cached_texcoords, tess::Painter::Buffer& buf, int begin, int end)
		//{
		//	assert(begin < end);

		//	float x = cached_texcoords[0];
		//	float y = cached_texcoords[1];
		//	float w = cached_texcoords[2] - cached_texcoords[0];
		//	float h = cached_texcoords[5] - cached_texcoords[1];

		//	auto v_ptr = &buf.vertices[begin];
		//	for (size_t i = 0, n = end - begin + 1; i < n; ++i)
		//	{
		//		auto& v = *v_ptr++;
		//		v.uv.x = x + w * v.uv.x;
		//		v.uv.y = y + h * v.uv.y;
		//	}
		//};

		//auto& buf = const_cast<tess::Painter::Buffer&>(pt.GetBuffer());
		//if (buf.vertices.empty()) {
		//	return;
		//}

		//auto& regions = pt.GetOtherTexRegion();
		//if (regions.empty())
		//{
		//	relocate_palette(cached_texcoords, buf, 0, buf.vertices.size() - 1);
		//}
		//else
		//{
		//	if (regions.front().begin > 0) {
		//		relocate_palette(cached_texcoords, buf, 0, regions.front().begin - 1);
		//	}
		//	for (int i = 0, n = regions.size(); i < n; ++i)
		//	{
		//		auto& r = regions[i];
		//		if (i > 0) {
		//			auto& prev = regions[i - 1];
		//			if (prev.end + 1 < r.begin) {
		//				relocate_palette(cached_texcoords, buf, prev.end + 1, r.begin - 1);
		//			}
		//		}
		//	}
		//	if (regions.back().end < static_cast<int>(buf.vertices.size() - 1)) {
		//		relocate_palette(cached_texcoords, buf, regions.back().end + 1, buf.vertices.size() - 1);
		//	}
		//}
	};

	egui::Callback::RegisterCallback(cb);
}

GUI::~GUI()
{
}

}