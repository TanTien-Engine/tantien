#include "modules/maths/wrap_Maths.h"
#include "modules/script/TransHelper.h"

#include <SM_Matrix2D.h>
#include <SM_Test.h>

#include <string.h>

namespace
{

void mat2_allocate()
{
    sm::Matrix2D* mt = (sm::Matrix2D*)ves_set_newforeign(0, 0, sizeof(sm::Matrix2D));
    mt->Identity();
}

int mat2_finalize(void* data)
{
    return sizeof(sm::Matrix2D);
}

void mat2_transform()
{
    sm::Matrix2D* mt = (sm::Matrix2D*)ves_toforeign(0);
    const float x = (float)ves_tonumber(1);
    const float y = (float)ves_tonumber(2);
    const float angle = (float)ves_tonumber(3);
    const float sx = (float)ves_tonumber(4);
    const float sy = (float)ves_tonumber(5);
    const float ox = (float)ves_tonumber(6);
    const float oy = (float)ves_tonumber(7);
    const float kx = (float)ves_tonumber(8);
    const float ky = (float)ves_tonumber(9);
    mt->SetTransformation(x, y, angle, sx, sy, ox, oy, kx, ky);
}

void f2_allocate()
{
    sm::vec2* v2 = (sm::vec2*)ves_set_newforeign(0, 0, sizeof(sm::vec2));
    v2->x = 0;
    v2->y = 0;
}

int f2_finalize(void* data)
{
    return sizeof(sm::vec2);
}

void f2_set_x()
{
    sm::vec2* v2 = (sm::vec2*)ves_toforeign(0);
    v2->x = (float)ves_tonumber(1);
}

void f2_get_x()
{
    sm::vec2* v2 = (sm::vec2*)ves_toforeign(0);
    ves_set_number(0, static_cast<double>(v2->x));
}

void f2_set_y()
{
    sm::vec2* v2 = (sm::vec2*)ves_toforeign(0);
    v2->y = (float)ves_tonumber(1);
}

void f2_get_y()
{
    sm::vec2* v2 = (sm::vec2*)ves_toforeign(0);
    ves_set_number(0, static_cast<double>(v2->y));
}

void f2_transform()
{
    sm::vec2* v2 = (sm::vec2*)ves_toforeign(0);
    sm::Matrix2D* mt = (sm::Matrix2D*)ves_toforeign(1);
    *v2 = *mt * *v2;
}

void maths_is_conv_intersect_conv()
{
    auto c0 = tt::list_to_vec2_array(1);
    auto c1 = tt::list_to_vec2_array(2);
    bool ret = sm::is_convex_intersect_convex(c0, c1);
    ves_set_boolean(0, ret);
}

}

namespace tt
{

VesselForeignMethodFn MathsBindMethod(const char* signature)
{
    if (strcmp(signature, "Matrix2D.transform(_,_,_,_,_,_,_,_,_)") == 0) return mat2_transform;

    //if (strcmp(signature, "Float2.x=(_)") == 0) return f2_set_x;
    //if (strcmp(signature, "Float2.x()") == 0) return f2_get_x;
    if (strcmp(signature, "Float2.setX(_)") == 0) return f2_set_x;
    if (strcmp(signature, "Float2.x()") == 0) return f2_get_x;
    if (strcmp(signature, "Float2.setY(_)") == 0) return f2_set_y;
    if (strcmp(signature, "Float2.y()") == 0) return f2_get_y;
    if (strcmp(signature, "Float2.transform(_)") == 0) return f2_transform;

    if (strcmp(signature, "static Maths.isConvexIntersectConvex(_,_)") == 0) return maths_is_conv_intersect_conv;

	return nullptr;
}

void MathsBindClass(const char* className, VesselForeignClassMethods* methods)
{
    if (strcmp(className, "Float2") == 0)
    {
        methods->allocate = f2_allocate;
        methods->finalize = f2_finalize;
        return;
    }

    if (strcmp(className, "Matrix2D") == 0)
    {
        methods->allocate = mat2_allocate;
        methods->finalize = mat2_finalize;
        return;
    }
}

}