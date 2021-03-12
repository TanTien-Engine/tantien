#include "modules/gui/wrap_GUI.h"
#include "modules/gui/GUI.h"
#include "modules/render/Render.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/SpriteRenderer.h"
#include "modules/script/Proxy.h"

#include <easygui/Context.h>
#include <easygui/ImGui.h>
#include <easygui/RenderBuffer.h>
#include <tessellation/Palette.h>
#include <unirender/Context.h>
#include <guard/check.h>

#include <string>

namespace
{

void w_Context_allocate()
{
	auto ctx = std::make_shared<egui::Context>(tt::Graphics::Instance()->GetSpriteRenderer()->GetPalette());
	auto proxy = (tt::Proxy<egui::Context>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<egui::Context>));
	proxy->obj = ctx;
}

int w_Context_finalize(void* data)
{
	auto proxy = (tt::Proxy<egui::Context>*)(data);
	proxy->~Proxy();
	return sizeof(tt::Proxy<egui::Context>);
}

void w_GUI_begin()
{
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;

	tt::GUI::Instance()->ResetUID();

	ctx->BeginDraw(*tt::Render::Instance()->Device());
}

void w_GUI_end()
{
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	ctx->EndDraw(
		*tt::Render::Instance()->Device(),
		*tt::Render::Instance()->Context(),
		tt::Graphics::Instance()->GetSpriteRenderer()->GetShader()
	);
}

void w_GUI_update()
{
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	ctx->Update(0.03f);
}

void w_GUI_rebuild()
{
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	ctx->rbuf.Rebuild();
}

void w_GUI_onSize()
{
	const float w = (float)ves_tonumber(1);
	const float h = (float)ves_tonumber(2);
	tt::GUI::Instance()->OnSize(w, h);
}

void w_GUI_transScrPosToProj()
{
	const float x = (float)ves_tonumber(1);
	const float y = (float)ves_tonumber(2);
	const float cam_x = (float)ves_tonumber(3);
	const float cam_y = (float)ves_tonumber(4);
	const float cam_scale = (float)ves_tonumber(5);
	auto proj = tt::GUI::Instance()->TransScreenToProj({ x, y }, { cam_x, cam_y }, cam_scale);
	ves_pop(6);
	ves_newlist(2);
	ves_pushnumber(proj.x);
	ves_seti(-2, 0);
	ves_pop(1);
	ves_pushnumber(proj.y);
	ves_seti(-2, 1);
	ves_pop(1);
}

enum MouseButton
{
	MOUSE_LEFT = 1,
	MOUSE_RIGHT
};

enum MouseAction
{
	MOUSE_DOWN = 0,
	MOUSE_UP,
	MOUSE_MOVE,
	MOUSE_DRAG
};

void w_GUI_mouseInput()
{
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const int btn = (int)ves_tonumber(2);
	const int action = (int)ves_tonumber(3);
	const float x = (float)ves_tonumber(4);
	const float y = (float)ves_tonumber(5);
	const float cam_x = (float)ves_tonumber(6);
	const float cam_y = (float)ves_tonumber(7);
	const float cam_scale = (float)ves_tonumber(8);
	const auto pos = tt::GUI::Instance()->TransScreenToProj({ x, y }, { cam_x, cam_y }, cam_scale);

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
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const float x = (float)ves_tonumber(2);
	const float y = (float)ves_tonumber(3);
	const float w = (float)ves_tonumber(4);
	const float h = (float)ves_tonumber(5);

	egui::frame(tt::GUI::Instance()->NextUID(), x, y, w, h, *ctx, true);
}

void w_GUI_button()
{
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* label = ves_tostring(2);
	const float x = (float)ves_tonumber(3);
	const float y = (float)ves_tonumber(4);
	const float w = (float)ves_tonumber(5);
	const float h = (float)ves_tonumber(6);

	ves_set_boolean(0, egui::button(tt::GUI::Instance()->NextUID(), label, x, y, w, h, *ctx, true));
}

void w_GUI_slider()
{
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* label   = ves_tostring(2);
	const float val     = (float)ves_tonumber(3);
	const float x       = (float)ves_tonumber(4);
	const float y       = (float)ves_tonumber(5);
	const float height  = (float)ves_tonumber(6);
	const float max     = (float)ves_tonumber(7);
	const bool verticle = ves_toboolean(8);

	const bool ret_old = val > max || val < 0;
	float ret = std::max(0.0f, std::min(val, max));
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
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* text = ves_tostring(2);
	const float x = (float)ves_tonumber(3);
	const float y = (float)ves_tonumber(4);

	egui::label(tt::GUI::Instance()->NextUID(), text, x, y, *ctx, true);
}

void w_GUI_checkbox()
{
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* label = ves_tostring(2);
	const bool  val   = ves_toboolean(3);
	const float x     = (float)ves_tonumber(4);
	const float y     = (float)ves_tonumber(5);
	
	bool ret = val;

	egui::checkbox(tt::GUI::Instance()->NextUID(), label, &ret, x, y, *ctx, true);

	ves_set_boolean(0, ret);
}

void w_GUI_radio_button()
{
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* label = ves_tostring(2);
	const bool  val   = ves_toboolean(3);
	const float x     = (float)ves_tonumber(4);
	const float y     = (float)ves_tonumber(5);

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
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const ArrowDir dir    = (ArrowDir)ves_tonumber(2);
	const float    x      = (float)ves_tonumber(3);
	const float    y      = (float)ves_tonumber(4);
	const float    height = (float)ves_tonumber(5);
	const bool     repeat = ves_toboolean(6);

	ves_set_boolean(0, egui::arrow_button(tt::GUI::Instance()->NextUID(), egui::Direction(dir), x, y, height, repeat, *ctx, true));
}

void w_GUI_selectable()
{
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
	const char* label  = ves_tostring(2);
	const bool  val    = ves_toboolean(3);
	const float x      = (float)ves_tonumber(4);
	const float y      = (float)ves_tonumber(5);
	const float length = (float)ves_tonumber(6);

	bool ret = val;

	egui::selectable(tt::GUI::Instance()->NextUID(), label, &ret, x, y, length, *ctx, true);

	ves_set_boolean(0, ret);
}

void w_GUI_combo()
{
	auto ctx = ((tt::Proxy<egui::Context>*)ves_toforeign(1))->obj;
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

	int ret = curr_item;

	egui::combo(tt::GUI::Instance()->NextUID(), label, &ret, items.data(), (int)items.size(), x, y, length, *ctx, true);
	ves_pop(num);

	ves_set_number(0, ret);
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

	if (strcmp(signature, "static GUI.onSize(_,_)") == 0) return w_GUI_onSize;
	if (strcmp(signature, "static GUI.transScrPosToProj(_,_,_,_,_)") == 0) return w_GUI_transScrPosToProj;
	if (strcmp(signature, "static GUI.mouseInput(_,_,_,_,_,_,_,_)") == 0) return w_GUI_mouseInput;

	if (strcmp(signature, "static GUI.frame(_,_,_,_,_)") == 0) return w_GUI_frame;
	if (strcmp(signature, "static GUI.button(_,_,_,_,_,_)") == 0) return w_GUI_button;
	if (strcmp(signature, "static GUI.slider(_,_,_,_,_,_,_,_)") == 0) return w_GUI_slider;
	if (strcmp(signature, "static GUI.label(_,_,_,_)") == 0) return w_GUI_label;
	if (strcmp(signature, "static GUI.checkbox(_,_,_,_,_)") == 0) return w_GUI_checkbox;
	if (strcmp(signature, "static GUI.radio_button(_,_,_,_,_)") == 0) return w_GUI_radio_button;
	if (strcmp(signature, "static GUI.arrow_button(_,_,_,_,_,_)") == 0) return w_GUI_arrow_button;
	if (strcmp(signature, "static GUI.selectable(_,_,_,_,_,_)") == 0) return w_GUI_selectable;
	if (strcmp(signature, "static GUI.combo(_,_,_,_,_,_,_)") == 0) return w_GUI_combo;

	return nullptr;
}

void GUIBindClass(const char* className, VesselForeignClassMethods* methods)
{
	if (strcmp(className, "Context") == 0)
	{
		methods->allocate = w_Context_allocate;
		methods->finalize = w_Context_finalize;
		return;
	}
}

}