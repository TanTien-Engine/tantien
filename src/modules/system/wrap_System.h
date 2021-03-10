#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn SystemBindMethod(const char* signature);
void SystemBindClass(const char* className, VesselForeignClassMethods* methods);

}