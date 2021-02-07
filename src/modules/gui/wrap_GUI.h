#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn GUIBindMethod(const char* signature);
void GUIBindClass(const char* className, VesselForeignClassMethods* methods);

}