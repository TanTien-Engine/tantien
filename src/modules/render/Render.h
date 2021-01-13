#pragma once

#include <vessel.h>

VesselForeignClassMethods RenderBindClass(const char* module, const char* class_name);
VesselForeignMethodFn RenderBindMethod(const char* class_name, bool is_static, const char* signature);
