#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn KeyboardBindMethod(const char* signature);
void KeyboardBindClass(const char* class_name, VesselForeignClassMethods* methods);

}