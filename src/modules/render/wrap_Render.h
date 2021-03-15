#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn RenderBindMethod(const char* signature);
void RenderBindClass(const char* class_name, VesselForeignClassMethods* methods);

}