#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn RegenBindMethod(const char* signature);
void RegenBindClass(const char* class_name, VesselForeignClassMethods* methods);

}