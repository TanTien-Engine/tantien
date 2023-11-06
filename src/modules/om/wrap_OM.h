#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn OmBindMethod(const char* signature);
void OmBindClass(const char* class_name, VesselForeignClassMethods* methods);

}