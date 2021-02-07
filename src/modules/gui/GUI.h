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

private:
	egui::Context       ctx;
	egui::EntityFactory factory;
	egui::CompStorage   storage;

	TT_SINGLETON_DECLARATION(GUI)

}; // GUI

}