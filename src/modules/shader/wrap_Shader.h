#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn ShaderBindMethod(const char* signature);
void ShaderBindClass(const char* class_name, VesselForeignClassMethods* methods);

}