#include "Render.h"

#include <unirender/Device.h>

namespace
{

std::shared_ptr<ur::Device> device = nullptr;

}

namespace tt
{
VesselForeignClassMethods RenderBindClass(const char* module, const char* class_name)
{
}

VesselForeignMethodFn RenderBindMethod(const char* class_name, bool is_static, const char* signature)
{
}
}