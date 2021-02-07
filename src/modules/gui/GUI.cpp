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
	//int id;
	//size_t w, h;
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
	auto tex = sr->GetPalette().GetTexture();
	TEXTURES[TexType::PALETTE].tex = tex;
	//TEXTURES[TexType::PALETTE].id = tex->GetTexID();
	//TEXTURES[TexType::PALETTE].w = tex->GetWidth();
	//TEXTURES[TexType::PALETTE].h = tex->GetHeight();
	TEXTURES[TexType::LABEL].tex = DTex::Instance()->GetGlyphFirstPageTex();

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

		GTxt::Instance()->Draw(ctx, label, st, mat, 0xffffffff, 0, 0, false, &pt, false);
	};
	cb.relocate_texcoords = [](tess::Painter& pt)
	{
		auto relocate_palette = [](const Texture& tex, tess::Painter::Buffer& buf, int begin, int end)
		{
			assert(begin < end);

			UID uid = ResourceUID::TexQuad(tex.tex->GetTexID(), 0, 0, tex.tex->GetWidth(), tex.tex->GetHeight());
			int block_id;
			ur::TexturePtr cached_tex = nullptr;
			auto cached_texcoords = DTex::Instance()->QuerySymbol(uid, cached_tex, block_id);
			if (cached_texcoords)
			{
				float x = cached_texcoords[0];
				float y = cached_texcoords[1];
				float w = cached_texcoords[2] - cached_texcoords[0];
				float h = cached_texcoords[5] - cached_texcoords[1];

				auto v_ptr = &buf.vertices[begin];
				for (size_t i = 0, n = end - begin + 1; i < n; ++i)
				{
					auto& v = *v_ptr++;
					v.uv.x = x + w * v.uv.x;
					v.uv.y = y + h * v.uv.y;
				}
			}
			else
			{
				UID uid = ResourceUID::TexQuad(tex.tex->GetTexID(), 0, 0, tex.tex->GetWidth(), tex.tex->GetHeight());
				LoadingList::Instance()->AddSymbol(uid, tex.tex, sm::irect(sm::ivec2(0, 0), sm::ivec2(tex.tex->GetWidth(), tex.tex->GetHeight())));
			}
		};
		auto relocate_label = [](const Texture& tex, tess::Painter::Buffer& buf, int begin, int end)
		{
			assert((end - begin + 1) % 4 == 0);
			for (int i = begin; i < end; i += 4)
			{
				auto min = buf.vertices[i].uv;
				auto max = buf.vertices[i + 2].uv;

				sm::rect r;
				r.xmin = tex.tex->GetWidth() * min.x;
				r.xmax = tex.tex->GetWidth() * max.x;
				r.ymin = tex.tex->GetHeight() * min.y;
				r.ymax = tex.tex->GetHeight() * max.y;

				sm::irect qr;
				qr.xmin = static_cast<int>(r.xmin);
				qr.xmax = static_cast<int>(r.xmax);
				qr.ymin = static_cast<int>(r.ymin);
				qr.ymax = static_cast<int>(r.ymax);

				UID uid = ResourceUID::TexQuad(tex.tex->GetTexID(), r.xmin, r.ymin, r.Width(), r.Height());
				int block_id;
				ur::TexturePtr cached_tex = nullptr;
				auto cached_texcoords = DTex::Instance()->QuerySymbol(uid, cached_tex, block_id);
				if (cached_texcoords)
				{
					//CURR_TEXID = cached_texid;
					auto v_ptr = &buf.vertices[i];
					v_ptr[0].uv.x = cached_texcoords[0];
					v_ptr[0].uv.y = cached_texcoords[1];
					v_ptr[1].uv.x = cached_texcoords[2];
					v_ptr[1].uv.y = cached_texcoords[3];
					v_ptr[2].uv.x = cached_texcoords[4];
					v_ptr[2].uv.y = cached_texcoords[5];
					v_ptr[3].uv.x = cached_texcoords[6];
					v_ptr[3].uv.y = cached_texcoords[7];
				}
				else
				{
					//rp::Callback::AddCacheSymbol(tex.id, tex.w, tex.h, qr);
					UID uid = ResourceUID::TexQuad(tex.tex->GetTexID(), r.xmin, r.ymin, r.Width(), r.Height());
					LoadingList::Instance()->AddSymbol(uid, tex.tex, qr);
				}
			}
		};

		auto& buf = const_cast<tess::Painter::Buffer&>(pt.GetBuffer());
		if (buf.vertices.empty()) {
			return;
		}

		auto& regions = pt.GetOtherTexRegion();
		if (regions.empty())
		{
			relocate_palette(TEXTURES[TexType::PALETTE], buf, 0, buf.vertices.size() - 1);
		}
		else
		{
			auto& palette = TEXTURES[TexType::PALETTE];
			auto& label = TEXTURES[TexType::LABEL];
			if (regions.front().begin > 0) {
				relocate_palette(palette, buf, 0, regions.front().begin - 1);
			}
			for (int i = 0, n = regions.size(); i < n; ++i)
			{
				auto& r = regions[i];
				if (i > 0) {
					auto& prev = regions[i - 1];
					if (prev.end + 1 < r.begin) {
						relocate_palette(palette, buf, prev.end + 1, r.begin - 1);
					}
				}
				assert(r.texid == label.tex->GetTexID());
				relocate_label(label, buf, r.begin, r.end);
			}
			if (regions.back().end < static_cast<int>(buf.vertices.size() - 1)) {
				relocate_palette(palette, buf, regions.back().end + 1, buf.vertices.size() - 1);
			}
		}
	};

	egui::Callback::RegisterCallback(cb);
}

GUI::~GUI()
{
}

}