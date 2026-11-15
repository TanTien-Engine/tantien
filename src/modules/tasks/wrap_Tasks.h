#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn TasksBindMethod(const char* signature);
void TasksBindClass(const char* class_name, VesselForeignClassMethods* methods);

}
