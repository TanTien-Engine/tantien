#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn SceneBindMethod(const char* signature);
void SceneBindClass(const char* class_name, VesselForeignClassMethods* methods);

}