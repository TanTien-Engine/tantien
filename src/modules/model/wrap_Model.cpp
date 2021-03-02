#include "modules/model/wrap_Model.h"

#include <model/ParametricEquations.h>

#include <memory>

#include <string.h>

namespace
{

}

namespace tt
{

VesselForeignMethodFn ModelBindMethod(const char* signature)
{
    return NULL;
}

void ModelBindClass(const char* className, VesselForeignClassMethods* methods)
{
}

}