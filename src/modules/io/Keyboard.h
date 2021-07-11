#pragma once

#include <functional>

namespace tt
{

class Keyboard
{
public:
	struct Funs
	{
		std::function<bool()> is_ctrl_pressed  = nullptr;
		std::function<bool()> is_shift_pressed = nullptr;
	};

	static void RegisterCallback(const Funs& funs);

	static bool IsCtrlPressed();
	static bool IsShiftPressed();

}; // Keyboard

}