#include "modules/geometry/wrap_Geometry.h"
#include "modules/script/TransHelper.h"
#include "modules/script/Proxy.h"

#include <geoshape/Bezier.h>
#include <guard/check.h>

#include <string>

namespace
{

void w_Bezier_allocate()
{
    auto proxy = (tt::Proxy<gs::Bezier>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<gs::Bezier>));
    proxy->obj = std::make_shared<gs::Bezier>();
}

int w_Bezier_finalize(void* data)
{
    auto proxy = (tt::Proxy<gs::Bezier>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<gs::Bezier>);
}

void w_Bezier_setCtrlPos()
{
    auto b = ((tt::Proxy<gs::Bezier>*)ves_toforeign(0))->obj;
    auto vertices = tt::list_to_vec2_array(1);
    GD_ASSERT(vertices.size() == 4, "error number");
    b->SetCtrlPos({ vertices[0], vertices[1], vertices[2], vertices[3] });
}

}

namespace tt
{

VesselForeignMethodFn GeometryBindMethod(const char* signature)
{
    if (strcmp(signature, "Bezier.setCtrlPos(_)") == 0) return w_Bezier_setCtrlPos;

    return nullptr;
}

void GeometryBindClass(const char* className, VesselForeignClassMethods* methods)
{
    if (strcmp(className, "Bezier") == 0)
    {
        methods->allocate = w_Bezier_allocate;
        methods->finalize = w_Bezier_finalize;
        return;
    }
}

}