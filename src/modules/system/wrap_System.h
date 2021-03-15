#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn SystemBindMethod(const char* signature);
void SystemBindClass(const char* class_name, VesselForeignClassMethods* methods);

}