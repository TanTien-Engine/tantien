#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn MathsBindMethod(const char* signature);
void MathsBindClass(const char* class_name, VesselForeignClassMethods* methods);

}