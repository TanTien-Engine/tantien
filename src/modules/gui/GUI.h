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

private:
	egui::Context       ctx;
	egui::EntityFactory factory;
	egui::CompStorage   storage;

	uint32_t m_next_uid = 1;

	TT_SINGLETON_DECLARATION(GUI)

}; // GUI

}