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

float SCREEN_WIDTH = 0;
float SCREEN_HEIGHT = 0;

sm::vec2 screen2proj(float x, float y)
{
	return sm::vec2(x - SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f - y);
}

void w_GUI_onSize()
{
	SCREEN_WIDTH = (float)ves_tonumber(1);
	SCREEN_HEIGHT = (float)ves_tonumber(2);

}

void w_GUI_mouseInput()
{
	const int btn = (int)ves_tonumber(1);
	const int action = (int)ves_tonumber(2);
	const float x = (float)ves_tonumber(3);
	const float y = (float)ves_tonumber(4);
	const auto pos = screen2proj(x, y);

	auto& ctx = tt::GUI::Instance()->GetContext();
	switch (btn)
	{
	case 1:
	{
		if (action == 0) {
			ctx.input_events.emplace_back(egui::InputType::MOUSE_LEFT_DOWN, pos.x, pos.y);
		} else if (action == 1) {
			ctx.input_events.emplace_back(egui::InputType::MOUSE_LEFT_UP, pos.x, pos.y);
		}
	}
		break;
	case 2:
	{
		if (action == 0) {
			ctx.input_events.emplace_back(egui::InputType::MOUSE_RIGHT_DOWN, pos.x, pos.y);
		} else if (action == 1) {
			ctx.input_events.emplace_back(egui::InputType::MOUSE_RIGHT_UP, pos.x, pos.y);
		}
	}
		break;
	}
}

void button()
{
	const char* label = ves_tostring(1);
	const float x = (float)ves_tonumber(2);
	const float y = (float)ves_tonumber(3);
	const float w = (float)ves_tonumber(4);
	const float h = (float)ves_tonumber(5);

	auto& ctx = tt::GUI::Instance()->GetContext();
	egui::button(tt::GUI::Instance()->NextUID(), label, x, y, w, h, ctx, true);
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
	if (strcmp(signature, "static GUI.mouseInput(_,_,_,_)") == 0) return w_GUI_mouseInput;

	if (strcmp(signature, "static GUI.button(_,_,_,_,_)") == 0) return button;

	return nullptr;
}

void GUIBindClass(const char* className, VesselForeignClassMethods* methods)
{
}

}