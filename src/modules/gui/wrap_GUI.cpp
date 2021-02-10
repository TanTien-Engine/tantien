#include "modules/gui/wrap_GUI.h"
#include "modules/gui/GUI.h"
#include "modules/render/Render.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/SpriteRenderer.h"

#include <easygui/ImGui.h>
#include <tessellation/Palette.h>
#include <unirender/Context.h>

#include <string>

namespace
{

void begin()
{
	tt::GUI::Instance()->ResetUID();

	auto& ctx = tt::GUI::Instance()->GetContext();
	ctx.BeginDraw(*tt::Render::Instance()->Device());
}

void end()
{
	auto& ctx = tt::GUI::Instance()->GetContext();
	ctx.EndDraw(
		*tt::Render::Instance()->Device(),
		*tt::Render::Instance()->Context(),
		tt::Graphics::Instance()->GetSpriteRenderer()->GetShader()
	);
}

void update()
{
	auto& ctx = tt::GUI::Instance()->GetContext();
	ctx.Update(0.03f);
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
	auto proj = tt::GUI::Instance()->TransScreenToProj({ x, y });
	ves_pop(3);
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
	const int btn = (int)ves_tonumber(1);
	const int action = (int)ves_tonumber(2);
	const float x = (float)ves_tonumber(3);
	const float y = (float)ves_tonumber(4);
	const auto pos = tt::GUI::Instance()->TransScreenToProj({ x, y });

	auto& ctx = tt::GUI::Instance()->GetContext();
	switch (btn)
	{
	case MOUSE_LEFT:
	{
		switch (action)
		{
		case MOUSE_DOWN:
			ctx.input_events.emplace_back(egui::InputType::MOUSE_LEFT_DOWN, (int)pos.x, (int)pos.y);
			break;
		case MOUSE_UP:
			ctx.input_events.emplace_back(egui::InputType::MOUSE_LEFT_UP, (int)pos.x, (int)pos.y);
			break;
		}
	}
	break;
	case MOUSE_RIGHT:
	{
		switch (action)
		{
		case MOUSE_DOWN:
			ctx.input_events.emplace_back(egui::InputType::MOUSE_RIGHT_DOWN, (int)pos.x, (int)pos.y);
			break;
		case MOUSE_UP:
			ctx.input_events.emplace_back(egui::InputType::MOUSE_RIGHT_UP, (int)pos.x, (int)pos.y);
			break;
		}
		break;
	}
	}
	switch (action)
	{
	case MOUSE_MOVE:
		ctx.input_events.emplace_back(egui::InputType::MOUSE_MOVE, (int)pos.x, (int)pos.y);
		break;
	case MOUSE_DRAG:
		ctx.input_events.emplace_back(egui::InputType::MOUSE_DRAG, (int)pos.x, (int)pos.y);
		break;
	}
}

void w_GUI_button()
{
	const char* label = ves_tostring(1);
	const float x = (float)ves_tonumber(2);
	const float y = (float)ves_tonumber(3);
	const float w = (float)ves_tonumber(4);
	const float h = (float)ves_tonumber(5);

	auto& ctx = tt::GUI::Instance()->GetContext();
	ves_set_boolean(0, egui::button(tt::GUI::Instance()->NextUID(), label, x, y, w, h, ctx, true));
}

void w_GUI_slider()
{
	const char* label   = ves_tostring(1);
	const float val     = (float)ves_tonumber(2);
	const float x       = (float)ves_tonumber(3);
	const float y       = (float)ves_tonumber(4);
	const float height  = (float)ves_tonumber(5);
	const float max     = (float)ves_tonumber(6);
	const bool verticle = ves_toboolean(7);

	float ret = val;

	auto& ctx = tt::GUI::Instance()->GetContext();
	egui::slider(tt::GUI::Instance()->NextUID(), label, &ret, x, y, height, max, verticle, ctx, true);

	ves_set_number(0, ret);
}

void w_GUI_label()
{
	const char* text = ves_tostring(1);
	const float x = (float)ves_tonumber(2);
	const float y = (float)ves_tonumber(3);
	auto& ctx = tt::GUI::Instance()->GetContext();
	egui::label(tt::GUI::Instance()->NextUID(), text, x, y, ctx, true);
}

void w_GUI_checkbox()
{
	const char* label = ves_tostring(1);
	const bool  val   = ves_toboolean(2);
	const float x     = (float)ves_tonumber(3);
	const float y     = (float)ves_tonumber(4);
	
	bool ret = val;

	auto& ctx = tt::GUI::Instance()->GetContext();
	egui::checkbox(tt::GUI::Instance()->NextUID(), label, &ret, x, y, ctx, true);

	ves_set_boolean(0, ret);
}

void w_GUI_radio_button()
{
	const char* label = ves_tostring(1);
	const bool  val   = ves_toboolean(2);
	const float x     = (float)ves_tonumber(3);
	const float y     = (float)ves_tonumber(4);

	auto& ctx = tt::GUI::Instance()->GetContext();
	ves_set_boolean(0, egui::radio_button(tt::GUI::Instance()->NextUID(), label, val, x, y, ctx, true));
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
	const ArrowDir dir    = (ArrowDir)ves_tonumber(1);
	const float    x      = (float)ves_tonumber(2);
	const float    y      = (float)ves_tonumber(3);
	const float    height = (float)ves_tonumber(4);
	const bool     repeat = ves_toboolean(5);

	auto& ctx = tt::GUI::Instance()->GetContext();
	ves_set_boolean(0, egui::arrow_button(tt::GUI::Instance()->NextUID(), egui::Direction(dir), x, y, height, repeat, ctx, true));
}

void w_GUI_selectable()
{
	const char* label  = ves_tostring(1);
	const bool  val    = ves_toboolean(2);
	const float x      = (float)ves_tonumber(3);
	const float y      = (float)ves_tonumber(4);
	const float length = (float)ves_tonumber(5);

	bool ret = val;

	auto& ctx = tt::GUI::Instance()->GetContext();
	egui::selectable(tt::GUI::Instance()->NextUID(), label, &ret, x, y, length, ctx, true);

	ves_set_boolean(0, ret);
}

void w_GUI_combo()
{
	const char* label     = ves_tostring(1);
	const int   curr_item = (int)ves_tonumber(2);
	const float x         = (float)ves_tonumber(4);
	const float y         = (float)ves_tonumber(5);
	const float length    = (float)ves_tonumber(6);

	std::vector<const char*> items;
	assert(ves_type(3) == VES_TYPE_LIST);
	const int num = ves_len(3);
	items.resize(num);
	for (int i = 0; i < num; ++i) {
		ves_geti(3, i);
		items[i] = ves_tostring(-1);
	}

	int ret = curr_item;

	auto& ctx = tt::GUI::Instance()->GetContext();
	egui::combo(tt::GUI::Instance()->NextUID(), label, &ret, items.data(), (int)items.size(), x, y, length, ctx, true);
	ves_pop(num);

	ves_set_number(0, ret);
}

}

namespace tt
{

VesselForeignMethodFn GUIBindMethod(const char* signature)
{
	if (strcmp(signature, "static GUI.begin()") == 0) return begin;
	if (strcmp(signature, "static GUI.end()") == 0) return end;
	if (strcmp(signature, "static GUI.update()") == 0) return update;

	if (strcmp(signature, "static GUI.onSize(_,_)") == 0) return w_GUI_onSize;
	if (strcmp(signature, "static GUI.transScrPosToProj(_,_)") == 0) return w_GUI_transScrPosToProj;
	if (strcmp(signature, "static GUI.mouseInput(_,_,_,_)") == 0) return w_GUI_mouseInput;

	if (strcmp(signature, "static GUI.button(_,_,_,_,_)") == 0) return w_GUI_button;
	if (strcmp(signature, "static GUI.slider(_,_,_,_,_,_,_)") == 0) return w_GUI_slider;
	if (strcmp(signature, "static GUI.label(_,_,_)") == 0) return w_GUI_label;
	if (strcmp(signature, "static GUI.checkbox(_,_,_,_)") == 0) return w_GUI_checkbox;
	if (strcmp(signature, "static GUI.radio_button(_,_,_,_)") == 0) return w_GUI_radio_button;
	if (strcmp(signature, "static GUI.arrow_button(_,_,_,_,_)") == 0) return w_GUI_arrow_button;
	if (strcmp(signature, "static GUI.selectable(_,_,_,_,_)") == 0) return w_GUI_selectable;
	if (strcmp(signature, "static GUI.combo(_,_,_,_,_,_)") == 0) return w_GUI_combo;

	return nullptr;
}

void GUIBindClass(const char* className, VesselForeignClassMethods* methods)
{
}

}