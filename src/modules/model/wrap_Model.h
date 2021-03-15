#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn ModelBindMethod(const char* signature);
void ModelBindClass(const char* class_name, VesselForeignClassMethods* methods);

}