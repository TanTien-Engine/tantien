#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn VmBindMethod(const char* signature);
void VmBindClass(const char* class_name, VesselForeignClassMethods* methods);

}