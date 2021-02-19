#include "modules/gui/GUI.h"
#include "modules/graphics/GTxt.h"
#include "modules/graphics/DTex.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/SpriteRenderer.h"
#include "modules/graphics/LoadingList.h"
#include "modules/filesystem/ResourceUID.h"

#include <easygui/Callback.h>

namespace tt
{

TT_SINGLETON_DEFINITION(GUI)

GUI::GUI()
{
	egui::style_colors_dark(ctx.style);

	ctx.SetPalette(Graphics::Instance()->GetSpriteRenderer()->GetPalette());

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
	};

	egui::Callback::RegisterCallback(cb);
}

GUI::~GUI()
{
}

sm::vec2 GUI::TransScreenToProj(const sm::vec2& screen) const
{
	float x = (screen.x - m_screen_width * 0.5f) / m_cam_scale - m_cam_offset.x;
	float y = (m_screen_height * 0.5f - screen.y) / m_cam_scale - m_cam_offset.y;
	return sm::vec2(x, y);
}

}