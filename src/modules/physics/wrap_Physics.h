#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn PhysicsBindMethod(const char* signature);
void PhysicsBindClass(const char* class_name, VesselForeignClassMethods* methods);

}