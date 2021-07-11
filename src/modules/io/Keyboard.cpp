#include "modules/io/Keyboard.h"

namespace tt
{

static Keyboard::Funs FUNS;

void Keyboard::RegisterCallback(const Keyboard::Funs& funs)
{
	FUNS = funs;
}

bool Keyboard::IsCtrlPressed()
{
	if (FUNS.is_ctrl_pressed) {
		return FUNS.is_ctrl_pressed();
	} else {
		return false;
	}
}

bool Keyboard::IsShiftPressed()
{
	if (FUNS.is_shift_pressed) {
		return FUNS.is_shift_pressed();
	} else {
		return false;
	}
}

}
