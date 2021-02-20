#pragma once

#include "core/macro.h"

#include <easygui/Context.h>
#include <easygui/EntityFactory.h>
#include <easygui/CompStorage.h>

#include <memory>

namespace tt
{

class GUI
{
public:
	auto& GetContext() { return ctx; }

	void ResetUID() { m_next_uid = 1; }
	uint32_t NextUID() { return m_next_uid++; }

	void OnSize(float width, float height) {
		m_screen_width = width;
		m_screen_height = height;
	}

	sm::vec2 TransScreenToProj(const sm::vec2& screen) const;
	void OnCameraUpdate(const sm::vec2& offset, float scale) {
		m_cam_offset = offset;
		m_cam_scale = scale;
	}

private:
	egui::Context       ctx;
	egui::EntityFactory factory;
	egui::CompStorage   storage;

	uint32_t m_next_uid = 1;

	float m_screen_width = 0, m_screen_height = 0;
	 
	sm::vec2 m_cam_offset;
	float m_cam_scale = 1.0f;

	TT_SINGLETON_DECLARATION(GUI)

}; // GUI

}