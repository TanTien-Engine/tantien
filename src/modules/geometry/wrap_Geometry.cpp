#include "modules/geometry/wrap_Geometry.h"
#include "modules/script/TransHelper.h"

#include <geoshape/Bezier.h>

#include <string>

namespace
{

void w_Bezier_allocate()
{
    auto b = new gs::Bezier();
    gs::Bezier** ptr = (gs::Bezier**)ves_set_newforeign(0, 0, sizeof(b));
    *ptr = b;
}

static int w_Bezier_finalize(void* data)
{
    gs::Bezier** ptr = static_cast<gs::Bezier**>(data);
    int ret = sizeof(*ptr);
    delete *ptr;
    return ret;
}

void w_Bezier_setCtrlPos()
{
    gs::Bezier* b = *(gs::Bezier**)ves_toforeign(0);
    auto vertices = tt::list_to_vec2_array(1);
    assert(vertices.size() == 4);
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