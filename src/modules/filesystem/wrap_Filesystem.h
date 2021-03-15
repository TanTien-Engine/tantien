#pragma once

#include <vessel.h>

namespace tt
{

VesselForeignMethodFn FilesystemBindMethod(const char* signature);
void FilesystemBindClass(const char* class_name, VesselForeignClassMethods* methods);

}