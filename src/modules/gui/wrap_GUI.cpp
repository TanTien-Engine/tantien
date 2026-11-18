#include "modules/gui/wrap_GUI.h"
#include "modules/gui/GUI.h"
#include "modules/render/Render.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/SpriteRenderer.h"

#include <easygui/Context.h>
#include <easygui/ImGui.h>
#include <easygui/RenderBuffer.h>
#include <easygui/Callback.h>
#include <easygui/Utility.h>
#include <tessellation/Palette.h>
#include <tessellation/Painter.h>
#include <unirender/Context.h>
#include <guard/check.h>
#include <wrapper/TransHelper.h>
#include <wrapper/Proxy.h>

#include <cmath>
#include <cstdlib>
#include <string>

namespace
{

// PanelKit hit-rect capture for the hand-written widgets below: the editor
// consults tt::GUI::kit().InputBlocked() before forwarding gestures to the
// canvas operators, so EVERY interactive widget must register its rect.
void KitRecord(float x, float y, float w, float h)
{
	tt::GUI::Instance()->kit().RecordWidget(x, y, w, h);
}

sm::vec2 KitLabelPad(const egui::Context& ctx, const char* label)
{
	return egui::Callback::GetLabelSize(label) + ctx.style.frame_padding * 2;
}

// Pack Vessel-side rgba (0..255 each) into the ABGR uint32 tess::Painter uses.
uint32_t PackColor(float r, float g, float b, float a)
{
	const int ri = static_cast<int>(r) < 0 ? 0 : (static_cast<int>(r) > 255 ? 255 : static_cast<int>(r));
	const int gi = static_cast<int>(g) < 0 ? 0 : (static_cast<int>(g) > 255 ? 255 : static_cast<int>(g));
	const int bi = static_cast<int>(b) < 0 ? 0 : (static_cast<int>(b) > 255 ? 255 : static_cast<int>(b));
	const int ai = static_cast<int>(a) < 0 ? 0 : (static_cast<int>(a) > 255 ? 255 : static_cast<int>(a));
	return (static_cast<uint32_t>(ai) << 24) | (static_cast<uint32_t>(bi) << 16)
		| (static_cast<uint32_t>(gi) << 8) | static_cast<uint32_t>(ri);
}

void AdvancePainter(egui::Context* ctx, uint32_t uid, tess::Painter& pt)
{
	ctx->rbuf.Advance(uid, pt);
}

egui::Context* Ctx(int stack_idx)
{
	return ((wrapper::Proxy<egui::Context>*)ves_toforeign(stack_idx))->obj.get();
}


void w_Context_allocate()
{
	auto ctx = std::make_shared<egui::Context>(tt::Graphics::Instance()->GetSpriteRenderer()->GetPalette());
	auto proxy = (wrapper::Proxy<egui::Context>*)ves_set_newforeign(0, 0, sizeof(wrapper::Proxy<egui::Context>));
	proxy->obj = ctx;
}

int w_Context_finalize(void* data)
{
	auto proxy = (wrapper::Proxy<egui::Context>*)(data);
	proxy->~Proxy();
	return sizeof(wrapper::Proxy<egui::Context>);
}

void w_GUI_begin()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;

	tt::GUI::Instance()->ResetUID();

	ctx->BeginDraw(*tt::Render::Instance()->Device());
}

void w_GUI_end()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	ctx->EndDraw(
		*tt::Render::Instance()->Device(),
		*tt::Render::Instance()->Context(),
		tt::Graphics::Instance()->GetSpriteRenderer()->GetShader()
	);
}

void w_GUI_update()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	ctx->Update(0.03f);
}

void w_GUI_rebuild()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	ctx->rbuf.Rebuild();
}

void w_GUI_onSize()
{
	const float w = (float)ves_tonumber(1);
	const float h = (float)ves_tonumber(2);
	tt::GUI::Instance()->OnSize(w, h);
}

void w_GUI_trans_scr_pos_to_proj()
{
	const float x = (float)ves_tonumber(1);
	const float y = (float)ves_tonumber(2);
	const float cam_x = (float)ves_tonumber(3);
	const float cam_y = (float)ves_tonumber(4);
	const float cam_scale = (float)ves_tonumber(5);
	auto proj = tt::GUI::Instance()->TransScreenToProj({ x, y }, { cam_x, cam_y }, cam_scale);

	wrapper::return_list(std::vector<float>{ proj.x, proj.y });
}

enum MouseButton
{
	MOUSE_LEFT = 1,
	MOUSE_RIGHT,
	MOUSE_MIDDLE,
};

enum MouseAction
{
	MOUSE_DOWN = 0,
	MOUSE_UP,
	MOUSE_MOVE,
	MOUSE_DRAG
};

void w_GUI_mouse_input()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const int btn = (int)ves_tonumber(2);
	const int action = (int)ves_tonumber(3);
	const float x = (float)ves_tonumber(4);
	const float y = (float)ves_tonumber(5);
	const float cam_x = (float)ves_tonumber(6);
	const float cam_y = (float)ves_tonumber(7);
	const float cam_scale = (float)ves_tonumber(8);
	const auto pos = tt::GUI::Instance()->TransScreenToProj({ x, y }, { cam_x, cam_y }, cam_scale);

	// PanelKit capture: mirror the pointer + drag-lock state so the editor
	// can keep panel gestures out of the node/3D canvas.
	tt::GUI::Instance()->kit().SetDragLocked(ctx->gui.IsDragLocked());
	tt::GUI::Instance()->kit().OnMouse(action, pos.x, pos.y);

	switch (btn)
	{
	case MOUSE_LEFT:
	{
		switch (action)
		{
		case MOUSE_DOWN:
			ctx->input_events.emplace_back(egui::InputType::MOUSE_LEFT_DOWN, (int)pos.x, (int)pos.y);
			break;
		case MOUSE_UP:
			ctx->input_events.emplace_back(egui::InputType::MOUSE_LEFT_UP, (int)pos.x, (int)pos.y);
			break;
		}
	}
		break;
	case MOUSE_RIGHT:
	{
		switch (action)
		{
		case MOUSE_DOWN:
			ctx->input_events.emplace_back(egui::InputType::MOUSE_RIGHT_DOWN, (int)pos.x, (int)pos.y);
			break;
		case MOUSE_UP:
			ctx->input_events.emplace_back(egui::InputType::MOUSE_RIGHT_UP, (int)pos.x, (int)pos.y);
			break;
		}
		break;
	}
	case MOUSE_MIDDLE:
	{
		switch (action)
		{
		case MOUSE_DOWN:
			ctx->input_events.emplace_back(egui::InputType::MOUSE_MIDDLE_DOWN, (int)pos.x, (int)pos.y);
			break;
		case MOUSE_UP:
			ctx->input_events.emplace_back(egui::InputType::MOUSE_MIDDLE_UP, (int)pos.x, (int)pos.y);
			break;
		}
		break;
	}
	}
	switch (action)
	{
	case MOUSE_MOVE:
		ctx->input_events.emplace_back(egui::InputType::MOUSE_MOVE, (int)pos.x, (int)pos.y);
		break;
	case MOUSE_DRAG:
		if (ctx->gui.IsDragLocked()) {
			ctx->input_events.emplace_back(egui::InputType::MOUSE_DRAG, (int)pos.x, (int)pos.y);
		}
		break;
	}
}

void w_GUI_frame()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const float x = (float)ves_tonumber(2);
	const float y = (float)ves_tonumber(3);
	const float w = (float)ves_tonumber(4);
	const float h = (float)ves_tonumber(5);

	egui::frame(tt::GUI::Instance()->NextUID(), x, y, w, h, *ctx, true);
}

void w_GUI_button()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* label = ves_tostring(2);
	const float x = (float)ves_tonumber(3);
	const float y = (float)ves_tonumber(4);

	// egui computes the button size from the label (+ padding); the script's
	// w/h arguments are ignored, so the hit rect mirrors the real size.
	const sm::vec2 tot = KitLabelPad(*ctx, label);
	KitRecord(x, y, tot.x, tot.y);

	ves_set_boolean(0, egui::button(tt::GUI::Instance()->NextUID(), label, x, y, tot.x, tot.y, *ctx, true));
}

void w_GUI_slider()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* label   = ves_tostring(2);
	const float val     = (float)ves_tonumber(3);
	const float x       = (float)ves_tonumber(4);
	const float y       = (float)ves_tonumber(5);
	const float height  = (float)ves_tonumber(6);
	const float max     = (float)ves_tonumber(7);
	const bool verticle = ves_toboolean(8);

	const bool ret_old = val > max || val < 0;
	float ret = std::max(0.0f, std::min(val, max));

	// Drag lock keeps the canvas operators out of the gesture while held.
	tt::GUI::Instance()->kit().SetDragLocked(ctx->gui.IsDragLocked());
	{
		const sm::vec2 lab = egui::Callback::GetLabelSize(label);
		if (!verticle) {
			KitRecord(x, y, height, lab.y + ctx->style.frame_padding.y * 2);
		} else {
			KitRecord(x, y, lab.y + ctx->style.frame_padding.x * 2, height);
		}
	}

	if (egui::slider(tt::GUI::Instance()->NextUID(), label, &ret, x, y, height, max, verticle, *ctx, true)) {
		if (ret_old) {
			ves_set_number(0, ves_tonumber(3));
		} else {
			ves_set_number(0, ret);
		}
	} else {
		ves_set_number(0, ves_tonumber(3));
	}
}

void w_GUI_label()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* text = ves_tostring(2);
	const float x = (float)ves_tonumber(3);
	const float y = (float)ves_tonumber(4);

	egui::label(tt::GUI::Instance()->NextUID(), text, x, y, *ctx, true);
}

void w_GUI_checkbox()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* label = ves_tostring(2);
	const bool  val   = ves_toboolean(3);
	const float x     = (float)ves_tonumber(4);
	const float y     = (float)ves_tonumber(5);

	{
		// Checkbox hit rect: label + a square box + paddings (see
		// Checkbox.cpp calc_tot_sz).
		const sm::vec2 lab = egui::Callback::GetLabelSize(label);
		KitRecord(x, y, lab.x + lab.y + ctx->style.frame_padding.x * 2,
			lab.y + ctx->style.frame_padding.y * 2);
	}

	bool ret = val;

	egui::checkbox(tt::GUI::Instance()->NextUID(), label, &ret, x, y, *ctx, true);

	ves_set_boolean(0, ret);
}

void w_GUI_radio_button()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* label = ves_tostring(2);
	const bool  val   = ves_toboolean(3);
	const float x     = (float)ves_tonumber(4);
	const float y     = (float)ves_tonumber(5);

	{
		const sm::vec2 lab = egui::Callback::GetLabelSize(label);
		KitRecord(x, y, lab.x + lab.y + ctx->style.frame_padding.x * 2,
			lab.y + ctx->style.frame_padding.y * 2);
	}

	ves_set_boolean(0, egui::radio_button(tt::GUI::Instance()->NextUID(), label, val, x, y, *ctx, true));
}

enum ArrowDir
{
	NONE = 0,
	LEFT,
	RIGHT,
	UP,
	DOWN,
};

void w_GUI_arrow_button()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const ArrowDir dir    = (ArrowDir)ves_tonumber(2);
	const float    x      = (float)ves_tonumber(3);
	const float    y      = (float)ves_tonumber(4);
	const float    height = (float)ves_tonumber(5);
	const bool     repeat = ves_toboolean(6);

	ves_set_boolean(0, egui::arrow_button(tt::GUI::Instance()->NextUID(), egui::Direction(dir), x, y, height, repeat, *ctx, true));
}

void w_GUI_selectable()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* label  = ves_tostring(2);
	const bool  val    = ves_toboolean(3);
	const float x      = (float)ves_tonumber(4);
	const float y      = (float)ves_tonumber(5);
	const float length = (float)ves_tonumber(6);

	{
		// Selectable hit rect: max(length, label + spacing) x label + vert
		// padding (see Selectable.cpp calc_tot_sz).
		const sm::vec2 lab = egui::Callback::GetLabelSize(label);
		const float w = length > lab.x + ctx->style.item_spacing.x
			? length : lab.x + ctx->style.item_spacing.x;
		KitRecord(x, y, w, lab.y + ctx->style.frame_padding.y * 2);
	}

	bool ret = val;

	// egui::selectable's return value is "the value CHANGED this frame"
	// (a click on any row, including the already-selected one) - that is
	// the click semantics callers want. Pushing `ret` (the VALUE) instead
	// made every already-selected row return true EVERY frame, so its
	// handler re-fired per frame (selection reverting, per-frame
	// renderbuffer rebuilds, egui input starvation - the stuck-click
	// family of bugs).
	bool changed = egui::selectable(tt::GUI::Instance()->NextUID(), label, &ret, x, y, length, *ctx, true);

	ves_set_boolean(0, changed);
}

void w_GUI_combo()
{
	auto ctx = ((wrapper::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* label     = ves_tostring(2);
	const int   curr_item = (int)ves_tonumber(3);
	const float x         = (float)ves_tonumber(5);
	const float y         = (float)ves_tonumber(6);
	const float length    = (float)ves_tonumber(7);

	std::vector<const char*> items;
	GD_ASSERT(ves_type(4) == VES_TYPE_LIST, "error type");
	const int num = ves_len(4);
	items.resize(num);
	for (int i = 0; i < num; ++i) {
		ves_geti(4, i);
		items[i] = ves_tostring(-1);
	}

	const uint32_t uid = tt::GUI::Instance()->NextUID();
	{
		// Header rect + the open popup rect (popup extends in -y above the
		// header, see Combo.cpp); both must capture the pointer.
		const sm::vec2 lab = egui::Callback::GetLabelSize(label);
		const float header_h = lab.y + ctx->style.frame_padding.y * 2;
		KitRecord(x, y, length, header_h);
		if (ctx->gui.GetPopupItem() == uid) {
			tt::GUI::Instance()->kit().RecordSurface(x, y - header_h * num, length, header_h * num);
		}
	}

	int ret = curr_item;

	egui::combo(uid, label, &ret, items.data(), (int)items.size(), x, y, length, *ctx, true);
	ves_pop(num);

	ves_set_number(0, ret);
}

}

// ===========================================================================
// PanelKit bindings: panels, clipping, scrolling, text/number fields, menus
// and the editor-facing capture queries. See modules/gui/PanelKit.h.
// ===========================================================================

namespace
{

// Per-draw-frame reset (NOT per context). editor.draw() calls this first so
// hit rects from both gui_top and gui_nodes accumulate into one frame list.
void w_GUI_frame_begin()
{
	tt::GUI::Instance()->kit().FrameBegin();
}

void w_GUI_push_id()
{
	tt::GUI::Instance()->kit().PushId((uint32_t)ves_tonumber(1));
}

void w_GUI_pop_id()
{
	tt::GUI::Instance()->kit().PopId();
}

void w_GUI_next_uid()
{
	ves_set_number(0, tt::GUI::Instance()->NextUID());
}

void w_GUI_input_blocked()
{
	ves_set_boolean(0, tt::GUI::Instance()->kit().InputBlocked());
}

void w_GUI_wants_keyboard()
{
	ves_set_boolean(0, tt::GUI::Instance()->kit().WantsKeyboard());
}

void w_GUI_key_input()
{
	const char* key = ves_tostring(1);
	ves_set_boolean(0, key ? tt::GUI::Instance()->kit().KeyInput(key) : false);
}

void w_GUI_wheel_input()
{
	// Screen coords in (GLFW, y-down); convert to gui_top projection space
	// like every other entry point, then route through the scroll stack.
	const float x = (float)ves_tonumber(1);
	const float y = (float)ves_tonumber(2);
	const float dx = (float)ves_tonumber(3);
	const float dy = (float)ves_tonumber(4);
	const auto pos = tt::GUI::Instance()->TransScreenToProj({ x, y }, { 0, 0 }, 1);
	ves_set_boolean(0, tt::GUI::Instance()->kit().Wheel(pos.x, pos.y, dx, dy));
}

// ---- panels ---------------------------------------------------------------

// Draws bg + header (with collapse toggle); kit-persists open state. Returns
// true when the caller should draw the panel content.
void w_GUI_panel_begin()
{
	auto ctx = Ctx(1);
	const uint32_t id = (uint32_t)ves_tonumber(2);
	const char* title = ves_tostring(3);
	const float x = (float)ves_tonumber(4);
	const float y = (float)ves_tonumber(5);
	const float w = (float)ves_tonumber(6);
	const float h = (float)ves_tonumber(7);

	auto& kit = tt::GUI::Instance()->kit();
	bool open = kit.PanelBegin(id, x, y, w, h);

	const sm::vec2 lab = egui::Callback::GetLabelSize(title);
	const float pad = 4.0f;
	const float header_h = lab.y + pad * 2;

	// header: visual top of the panel = larger y in this y-up space
	const float header_y = y + h - header_h;

	auto pt = ctx->NewPainter();
	pt.AddRectFilled(sm::vec2(x, y), sm::vec2(x + w, y + h),
		ctx->style.colors[(int)egui::Color::WindowBg], 0, 0);
	pt.AddRectFilled(sm::vec2(x, header_y), sm::vec2(x + w, y + h),
		ctx->style.colors[(int)egui::Color::Header], 0, 0);
	egui::render_text(*ctx->ctx, pt, title, x + pad, header_y + pad, lab.y, ctx->style);
	AdvancePainter(ctx, id, pt);

	// collapse toggle over the "+/-" glyph (egui buttons fire on DOWN)
	if (w > 30)
	{
		const uint32_t toggle_id = tt::GUI::Instance()->NextUID();
		KitRecord(x + w - 24, header_y, 24, header_h);
		if (egui::button(toggle_id, open ? "-" : "+", x + w - 24, header_y, 0, 0, *ctx, true)) {
			kit.PanelSetOpen(id, !open);
		}
	}

	ves_set_boolean(0, open);
}

void w_GUI_panel_end()
{
	// Symmetry only; per-panel state lives in the kit and script locals.
}

// ---- clipping (culling) ----------------------------------------------------

void w_GUI_clip_begin()
{
	const float x = (float)ves_tonumber(2);
	const float y = (float)ves_tonumber(3);
	const float w = (float)ves_tonumber(4);
	const float h = (float)ves_tonumber(5);
	tt::GUI::Instance()->kit().ClipBegin(x, y, w, h);
}

void w_GUI_clip_end()
{
	tt::GUI::Instance()->kit().ClipEnd();
}

void w_GUI_clip_culls()
{
	const float x = (float)ves_tonumber(2);
	const float y = (float)ves_tonumber(3);
	const float w = (float)ves_tonumber(4);
	const float h = (float)ves_tonumber(5);
	ves_set_boolean(0, tt::GUI::Instance()->kit().ClipCulls(x, y, w, h));
}

void w_GUI_clip_mouse()
{
	ves_set_boolean(0, tt::GUI::Instance()->kit().ClipContainsMouse());
}

// ---- scrolling ---------------------------------------------------------------

void w_GUI_scroll_begin()
{
	const uint32_t id = (uint32_t)ves_tonumber(2);
	const float x = (float)ves_tonumber(3);
	const float y = (float)ves_tonumber(4);
	const float w = (float)ves_tonumber(5);
	const float h = (float)ves_tonumber(6);
	const float content_h = (float)ves_tonumber(7);
	ves_set_number(0, tt::GUI::Instance()->kit().ScrollBegin(id, x, y, w, h, content_h));
}

void w_GUI_scroll_end()
{
	tt::GUI::Instance()->kit().ScrollEnd();
}

// Programmatic locate: set the stored offset of one scroll section so the
// UI can scroll a target row into view (feature navigation).
void w_GUI_scroll_set()
{
	const uint32_t id = (uint32_t)ves_tonumber(2);
	const float offset = (float)ves_tonumber(3);
	tt::GUI::Instance()->kit().ScrollSet(id, offset);
}

void w_GUI_scroll_viewport()
{
	float x = 0, y = 0, w = 0, h = 0;
	tt::GUI::Instance()->kit().ScrollViewport(&x, &y, &w, &h);
	ves_newlist(4);
	ves_pushnumber(x); ves_seti(-2, 0); ves_pop(1);
	ves_pushnumber(y); ves_seti(-2, 1); ves_pop(1);
	ves_pushnumber(w); ves_seti(-2, 2); ves_pop(1);
	ves_pushnumber(h); ves_seti(-2, 3); ves_pop(1);
}

// Draws the scrollbar for the ACTIVE scroll section and advances its drag
// state. bar_w is the track column width; content_h must be the same value
// passed to scroll_begin. Returns the (possibly new) offset.
void w_GUI_scrollbar()
{
	auto ctx = Ctx(1);
	const uint32_t id = (uint32_t)ves_tonumber(2);
	const float bar_w = (float)ves_tonumber(3);
	const float content_h = (float)ves_tonumber(4);

	auto& kit = tt::GUI::Instance()->kit();
	float vx = 0, vy = 0, vw = 0, vh = 0;
	kit.ScrollViewport(&vx, &vy, &vw, &vh);
	if (vh <= 0 || content_h <= vh) { ves_set_number(0, 0); return; }

	const float bar_x = vx + vw - bar_w;
	const float off = kit.ScrollbarDrag(id, bar_x, bar_w);

	// Track (always) + proportional grab; geometry mirrors
	// PanelKit::ScrollbarDrag (grab_h = vh * vh / content_h).
	const float max_off = content_h - vh;
	const float grab_h = vh * vh / content_h;
	const float grab_y = vy + (max_off > 0 ? off / max_off : 0.0f) * (vh - grab_h);

	const uint32_t uid = tt::GUI::Instance()->NextUID();
	auto pt = ctx->NewPainter();
	pt.AddRectFilled(sm::vec2(bar_x, vy), sm::vec2(bar_x + bar_w, vy + vh),
		ctx->style.colors[(int)egui::Color::ScrollbarBg], 0, 0);
	const egui::Color grab_col = kit.ScrollbarDragging()
		? egui::Color::ScrollbarGrabActive : egui::Color::ScrollbarGrab;
	pt.AddRectFilled(sm::vec2(bar_x + 1, grab_y), sm::vec2(bar_x + bar_w - 1, grab_y + grab_h),
		ctx->style.colors[(int)grab_col], 0, 0);
	AdvancePainter(ctx, uid, pt);

	ves_set_number(0, off);
}

void w_GUI_scrollbar_release()
{
	tt::GUI::Instance()->kit().ScrollbarRelease();
}

// ---- fields -----------------------------------------------------------------

void w_GUI_text_field()
{
	auto ctx = Ctx(1);
	const char* value = ves_tostring(2);
	const float x = (float)ves_tonumber(3);
	const float y = (float)ves_tonumber(4);
	const float w = (float)ves_tonumber(5);
	const float h = (float)ves_tonumber(6);
	const std::string val = value ? value : "";

	auto& kit = tt::GUI::Instance()->kit();
	const uint32_t uid = tt::GUI::Instance()->NextUID();

	std::string committed;
	const bool did_commit = kit.TextField(uid, x, y, w, h, val, &committed);
	KitRecord(x, y, w, h);

	const bool focused = kit.FocusId() == uid;
	const std::string& shown = focused ? kit.FocusBuffer() : val;

	// Field rendering: FrameBg fill + border (active when focused) + text.
	auto pt = ctx->NewPainter();
	const sm::vec2 min(x, y), max(x + w, y + h);
	pt.AddRectFilled(min, max, ctx->style.colors[(int)egui::Color::FrameBg], 0, 0);
	const uint32_t border_col = focused
		? ctx->style.colors[(int)egui::Color::FrameBgActive]
		: ctx->style.colors[(int)egui::Color::Border];
	pt.AddRect(min, max, border_col, 1, 0, 0);
	const sm::vec2 lab = egui::Callback::GetLabelSize(shown.c_str());
	egui::render_text(*ctx->ctx, pt, shown.c_str(), x + 4, y + (h - lab.y) * 0.5f, lab.y, ctx->style);
	if (focused)
	{
		// Caret: 1.5px bar after the text prefix before the caret index.
		const int caret = kit.FocusCaret();
		const std::string prefix = shown.substr(0, static_cast<size_t>(caret));
		const sm::vec2 pre = egui::Callback::GetLabelSize(prefix.c_str());
		const float cx = x + 4 + pre.x;
		pt.AddRectFilled(sm::vec2(cx, y + 2), sm::vec2(cx + 1.5f, y + h - 2),
			ctx->style.colors[(int)egui::Color::Text], 0, 0);
	}
	AdvancePainter(ctx, uid, pt);

	if (did_commit) {
		ves_set_lstring(0, committed.data(), (int)committed.size());
	} else {
		ves_set_nil(0);
	}
}

void w_GUI_number_field()
{
	auto ctx = Ctx(1);
	const double value = ves_tonumber(2);
	const float x = (float)ves_tonumber(3);
	const float y = (float)ves_tonumber(4);
	const float w = (float)ves_tonumber(5);
	const float h = (float)ves_tonumber(6);

	// Display: compact decimal, no trailing zeros (mm-scale CAD numbers).
	char buf[32];
	snprintf(buf, sizeof(buf), "%g", value);

	const uint32_t uid = tt::GUI::Instance()->NextUID();
	auto& kit = tt::GUI::Instance()->kit();
	std::string committed;
	const bool did_commit = kit.TextField(uid, x, y, w, h, buf, &committed);
	KitRecord(x, y, w, h);

	const bool focused = kit.FocusId() == uid;
	const std::string shown = focused ? kit.FocusBuffer() : std::string(buf);

	auto pt = ctx->NewPainter();
	const sm::vec2 min(x, y), max(x + w, y + h);
	pt.AddRectFilled(min, max, ctx->style.colors[(int)egui::Color::FrameBg], 0, 0);
	const uint32_t border_col = focused
		? ctx->style.colors[(int)egui::Color::FrameBgActive]
		: ctx->style.colors[(int)egui::Color::Border];
	pt.AddRect(min, max, border_col, 1, 0, 0);
	const sm::vec2 lab = egui::Callback::GetLabelSize(shown.c_str());
	egui::render_text(*ctx->ctx, pt, shown.c_str(), x + 4, y + (h - lab.y) * 0.5f, lab.y, ctx->style);
	if (focused)
	{
		const int caret = kit.FocusCaret();
		const std::string prefix = shown.substr(0, static_cast<size_t>(caret));
		const sm::vec2 pre = egui::Callback::GetLabelSize(prefix.c_str());
		const float cx = x + 4 + pre.x;
		pt.AddRectFilled(sm::vec2(cx, y + 2), sm::vec2(cx + 1.5f, y + h - 2),
			ctx->style.colors[(int)egui::Color::Text], 0, 0);
	}
	AdvancePainter(ctx, uid, pt);

	ves_newlist(2);
	if (did_commit)
	{
		// Parse the committed text: full-string numeric, optional whitespace
		// or a unit suffix (letters) after the number. On failure: no change
		// (the caller keeps the old value and may flag the input).
		const std::string s = committed;
		char* endp = nullptr;
		const double parsed = std::strtod(s.c_str(), &endp);
		bool ok = endp && endp > s.c_str();
		if (ok) {
			for (const char* p = endp; *p; ++p) {
				const unsigned char c = static_cast<unsigned char>(*p);
				if (!(c == ' ' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
					ok = false;
					break;
				}
			}
		}
		if (!(parsed == parsed)) { ok = false; }   // NaN
		ves_pushboolean(ok);
		ves_seti(-2, 0);
		ves_pop(1);
		ves_pushnumber(ok ? parsed : value);
		ves_seti(-2, 1);
		ves_pop(1);
	}
	else
	{
		ves_pushboolean(false);
		ves_seti(-2, 0);
		ves_pop(1);
		ves_pushnumber(value);
		ves_seti(-2, 1);
		ves_pop(1);
	}
}

// ---- primitive draws ---------------------------------------------------------

void w_GUI_rect_filled()
{
	auto ctx = Ctx(1);
	const float x = (float)ves_tonumber(2);
	const float y = (float)ves_tonumber(3);
	const float w = (float)ves_tonumber(4);
	const float h = (float)ves_tonumber(5);
	const uint32_t col = PackColor((float)ves_tonumber(6), (float)ves_tonumber(7),
		(float)ves_tonumber(8), (float)ves_tonumber(9));
	const uint32_t uid = tt::GUI::Instance()->NextUID();
	auto pt = ctx->NewPainter();
	pt.AddRectFilled(sm::vec2(x, y), sm::vec2(x + w, y + h), col, 0, 0);
	AdvancePainter(ctx, uid, pt);
}

void w_GUI_rect_outline()
{
	auto ctx = Ctx(1);
	const float x = (float)ves_tonumber(2);
	const float y = (float)ves_tonumber(3);
	const float w = (float)ves_tonumber(4);
	const float h = (float)ves_tonumber(5);
	const uint32_t col = PackColor((float)ves_tonumber(6), (float)ves_tonumber(7),
		(float)ves_tonumber(8), (float)ves_tonumber(9));
	const uint32_t uid = tt::GUI::Instance()->NextUID();
	auto pt = ctx->NewPainter();
	pt.AddRect(sm::vec2(x, y), sm::vec2(x + w, y + h), col, 1, 0, 0);
	AdvancePainter(ctx, uid, pt);
}

void w_GUI_progress()
{
	auto ctx = Ctx(1);
	const float x = (float)ves_tonumber(2);
	const float y = (float)ves_tonumber(3);
	const float w = (float)ves_tonumber(4);
	const float h = (float)ves_tonumber(5);
	float frac = (float)ves_tonumber(6);
	const uint32_t col = PackColor((float)ves_tonumber(7), (float)ves_tonumber(8),
		(float)ves_tonumber(9), (float)ves_tonumber(10));
	if (frac < 0) { frac = 0; }
	if (frac > 1) { frac = 1; }
	const uint32_t uid = tt::GUI::Instance()->NextUID();
	auto pt = ctx->NewPainter();
	pt.AddRectFilled(sm::vec2(x, y), sm::vec2(x + w, y + h),
		ctx->style.colors[(int)egui::Color::FrameBg], 0, 0);
	pt.AddRectFilled(sm::vec2(x, y), sm::vec2(x + w * frac, y + h), col, 0, 0);
	AdvancePainter(ctx, uid, pt);
}

// ---- menus -------------------------------------------------------------------

void w_GUI_menu_begin()
{
	const uint32_t id = (uint32_t)ves_tonumber(2);
	const float x = (float)ves_tonumber(3);
	const float y = (float)ves_tonumber(4);
	const float w = (float)ves_tonumber(5);
	const float h = (float)ves_tonumber(6);
	ves_set_boolean(0, tt::GUI::Instance()->kit().MenuBegin(id, x, y, w, h));
}

void w_GUI_menu_item()
{
	auto ctx = Ctx(1);
	const char* label = ves_tostring(2);
	const float x = (float)ves_tonumber(3);
	const float y = (float)ves_tonumber(4);
	const float w = (float)ves_tonumber(5);
	const float h = (float)ves_tonumber(6);

	const uint32_t uid = tt::GUI::Instance()->NextUID();
	const bool clicked = tt::GUI::Instance()->kit().MenuItem(x, y, w, h);

	auto pt = ctx->NewPainter();
	pt.AddRectFilled(sm::vec2(x, y), sm::vec2(x + w, y + h),
		ctx->style.colors[(int)egui::Color::PopupBg], 0, 0);
	const sm::vec2 lab = egui::Callback::GetLabelSize(label);
	egui::render_text(*ctx->ctx, pt, label, x + 6, y + (h - lab.y) * 0.5f, lab.y, ctx->style);
	AdvancePainter(ctx, uid, pt);

	ves_set_boolean(0, clicked);
}

void w_GUI_menu_end()
{
	tt::GUI::Instance()->kit().MenuEnd();
}

}

namespace tt
{

VesselForeignMethodFn GUIBindMethod(const char* signature)
{
	if (strcmp(signature, "static GUI.begin(_)") == 0) return w_GUI_begin;
	if (strcmp(signature, "static GUI.end(_)") == 0) return w_GUI_end;
	if (strcmp(signature, "static GUI.update(_)") == 0) return w_GUI_update;
	if (strcmp(signature, "static GUI.rebuild(_)") == 0) return w_GUI_rebuild;

	if (strcmp(signature, "static GUI.on_size(_,_)") == 0) return w_GUI_onSize;
	if (strcmp(signature, "static GUI.trans_scr_pos_to_proj(_,_,_,_,_)") == 0) return w_GUI_trans_scr_pos_to_proj;
	if (strcmp(signature, "static GUI.mouse_input(_,_,_,_,_,_,_,_)") == 0) return w_GUI_mouse_input;

	if (strcmp(signature, "static GUI.frame(_,_,_,_,_)") == 0) return w_GUI_frame;
	if (strcmp(signature, "static GUI.button(_,_,_,_,_,_)") == 0) return w_GUI_button;
	if (strcmp(signature, "static GUI.slider(_,_,_,_,_,_,_,_)") == 0) return w_GUI_slider;
	if (strcmp(signature, "static GUI.label(_,_,_,_)") == 0) return w_GUI_label;
	if (strcmp(signature, "static GUI.checkbox(_,_,_,_,_)") == 0) return w_GUI_checkbox;
	if (strcmp(signature, "static GUI.radio_button(_,_,_,_,_)") == 0) return w_GUI_radio_button;
	if (strcmp(signature, "static GUI.arrow_button(_,_,_,_,_,_)") == 0) return w_GUI_arrow_button;
	if (strcmp(signature, "static GUI.selectable(_,_,_,_,_,_)") == 0) return w_GUI_selectable;
	if (strcmp(signature, "static GUI.combo(_,_,_,_,_,_,_)") == 0) return w_GUI_combo;

	// ---- PanelKit: capture, panels, clip, scroll, fields, menus ----------
	if (strcmp(signature, "static GUI.frame_begin()") == 0) return w_GUI_frame_begin;
	if (strcmp(signature, "static GUI.push_id(_)") == 0) return w_GUI_push_id;
	if (strcmp(signature, "static GUI.pop_id()") == 0) return w_GUI_pop_id;
	if (strcmp(signature, "static GUI.next_uid()") == 0) return w_GUI_next_uid;

	if (strcmp(signature, "static GUI.input_blocked()") == 0) return w_GUI_input_blocked;
	if (strcmp(signature, "static GUI.wants_keyboard()") == 0) return w_GUI_wants_keyboard;
	if (strcmp(signature, "static GUI.key_input(_)") == 0) return w_GUI_key_input;
	if (strcmp(signature, "static GUI.wheel_input(_,_,_,_)") == 0) return w_GUI_wheel_input;

	if (strcmp(signature, "static GUI.panel_begin(_,_,_,_,_,_,_)") == 0) return w_GUI_panel_begin;
	if (strcmp(signature, "static GUI.panel_end(_)") == 0) return w_GUI_panel_end;

	if (strcmp(signature, "static GUI.clip_begin(_,_,_,_,_)") == 0) return w_GUI_clip_begin;
	if (strcmp(signature, "static GUI.clip_end(_)") == 0) return w_GUI_clip_end;
	if (strcmp(signature, "static GUI.clip_culls(_,_,_,_,_)") == 0) return w_GUI_clip_culls;
	if (strcmp(signature, "static GUI.clip_mouse(_)") == 0) return w_GUI_clip_mouse;

	if (strcmp(signature, "static GUI.scroll_begin(_,_,_,_,_,_,_)") == 0) return w_GUI_scroll_begin;
	if (strcmp(signature, "static GUI.scroll_end(_)") == 0) return w_GUI_scroll_end;
	if (strcmp(signature, "static GUI.scroll_set(_,_,_)") == 0) return w_GUI_scroll_set;
	if (strcmp(signature, "static GUI.scroll_viewport(_)") == 0) return w_GUI_scroll_viewport;
	if (strcmp(signature, "static GUI.scrollbar(_,_,_,_)") == 0) return w_GUI_scrollbar;
	if (strcmp(signature, "static GUI.scrollbar_release()") == 0) return w_GUI_scrollbar_release;

	if (strcmp(signature, "static GUI.text_field(_,_,_,_,_,_)") == 0) return w_GUI_text_field;
	if (strcmp(signature, "static GUI.number_field(_,_,_,_,_,_)") == 0) return w_GUI_number_field;

	if (strcmp(signature, "static GUI.rect_filled(_,_,_,_,_,_,_,_,_)") == 0) return w_GUI_rect_filled;
	if (strcmp(signature, "static GUI.rect_outline(_,_,_,_,_,_,_,_,_)") == 0) return w_GUI_rect_outline;
	if (strcmp(signature, "static GUI.progress(_,_,_,_,_,_,_,_,_,_)") == 0) return w_GUI_progress;

	if (strcmp(signature, "static GUI.menu_begin(_,_,_,_,_,_)") == 0) return w_GUI_menu_begin;
	if (strcmp(signature, "static GUI.menu_item(_,_,_,_,_,_)") == 0) return w_GUI_menu_item;
	if (strcmp(signature, "static GUI.menu_end()") == 0) return w_GUI_menu_end;

	return nullptr;
}

void GUIBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
	if (strcmp(class_name, "Context") == 0)
	{
		methods->allocate = w_Context_allocate;
		methods->finalize = w_Context_finalize;
		return;
	}
}

}