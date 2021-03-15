#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn ImageBindMethod(const char* signature);
void ImageBindClass(const char* class_name, VesselForeignClassMethods* methods);

}