#include "modules/io/wrap_Keyboard.h"
#include "modules/io/Keyboard.h"

#include <string.h>

namespace
{

void w_Keyboard_is_ctrl_pressed()
{
    ves_set_boolean(0, tt::Keyboard::IsCtrlPressed());
}

void w_Keyboard_is_shift_pressed()
{
    ves_set_boolean(0, tt::Keyboard::IsShiftPressed());
}

}

namespace tt
{

VesselForeignMethodFn KeyboardBindMethod(const char* signature)
{
    if (strcmp(signature, "static Keyboard.is_ctrl_pressed()") == 0) return w_Keyboard_is_ctrl_pressed;
    if (strcmp(signature, "static Keyboard.is_shift_pressed()") == 0) return w_Keyboard_is_shift_pressed;

    return NULL;
}

void KeyboardBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
}

}