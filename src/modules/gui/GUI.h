#pragma once

#include "core/macro.h"
#include "modules/gui/PanelKit.h"

#include <easygui/Context.h>
#include <easygui/EntityFactory.h>
#include <easygui/CompStorage.h>

#include <memory>

namespace tt
{

class GUI
{
public:
	void ResetUID() { m_next_uid = 1; }
	uint32_t NextUID() { return m_kit.NextScopedId(); }

	// Panel toolkit shared by BOTH gui contexts (single hit-rect list for
	// the whole frame; see PanelKit.h).
	PanelKit& kit() { return m_kit; }

	void OnSize(float width, float height) {
		m_screen_width = width;
		m_screen_height = height;
	}

	float ScreenWidth() const { return m_screen_width; }
	float ScreenHeight() const { return m_screen_height; }

	sm::vec2 TransScreenToProj(const sm::vec2& screen, const sm::vec2& cam_offset, float cam_scale) const;

private:
	egui::EntityFactory factory;
	egui::CompStorage   storage;

	PanelKit m_kit;

	uint32_t m_next_uid = 1;

	float m_screen_width = 0, m_screen_height = 0;

	TT_SINGLETON_DECLARATION(GUI)

}; // GUI

}
