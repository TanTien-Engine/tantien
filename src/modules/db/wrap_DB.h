#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn DbBindMethod(const char* signature);
void DbBindClass(const char* class_name, VesselForeignClassMethods* methods);

}