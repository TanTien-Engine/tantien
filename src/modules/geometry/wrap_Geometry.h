#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn GeometryBindMethod(const char* signature);
void GeometryBindClass(const char* class_name, VesselForeignClassMethods* methods);

}