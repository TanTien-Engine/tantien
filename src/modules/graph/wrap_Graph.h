#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn GraphBindMethod(const char* signature);
void GraphBindClass(const char* class_name, VesselForeignClassMethods* methods);

}