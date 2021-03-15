#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn GraphicsBindMethod(const char* signature);
void GraphicsBindClass(const char* class_name, VesselForeignClassMethods* methods);

}