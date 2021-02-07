#include "modules/gui/wrap_GUI.h"
#include "modules/gui/GUI.h"
#include "modules/render/Render.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/SpriteRenderer.h"

#include <easygui/ImGui.h>
#include <tessellation/Palette.h>

#include <string>

namespace
{

void begin()
{
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

void button()
{
	uint32_t uid = 1;
	auto& ctx = tt::GUI::Instance()->GetContext();
	egui::button(uid++, "btn0", 0, 0, 100, 50, ctx, true);
}

}

namespace tt
{

VesselForeignMethodFn GUIBindMethod(const char* signature)
{
	if (strcmp(signature, "static GUI.begin()") == 0) return begin;
	if (strcmp(signature, "static GUI.end()") == 0) return end;
	if (strcmp(signature, "static GUI.update()") == 0) return update;

	if (strcmp(signature, "static GUI.button(_,_,_,_,_)") == 0) return button;

	return nullptr;
}

void GUIBindClass(const char* className, VesselForeignClassMethods* methods)
{
}

}