#include "modules/maths/wrap_Maths.h"
#include "modules/script/TransHelper.h"
#include "modules/graphics/Graphics.h"

#include <SM_Matrix2D.h>
#include <SM_Test.h>
#include <SM_Matrix.h>

#include <string.h>

namespace
{

void w_Matrix2D_allocate()
{
    sm::Matrix2D* mt = (sm::Matrix2D*)ves_set_newforeign(0, 0, sizeof(sm::Matrix2D));
    mt->Identity();
}

int w_Matrix2D_finalize(void* data)
{
    return sizeof(sm::Matrix2D);
}

void w_Matrix44_allocate()
{
    sm::mat4* mt = (sm::mat4*)ves_set_newforeign(0, 0, sizeof(sm::mat4));
    mt->Identity();
}

int w_Matrix44_finalize(void* data)
{
    return sizeof(sm::mat4);
}

void w_Float2_allocate()
{
    sm::vec2* v2 = (sm::vec2*)ves_set_newforeign(0, 0, sizeof(sm::vec2));
    v2->x = 0;
    v2->y = 0;
}

int w_Float2_finalize(void* data)
{
    return sizeof(sm::vec2);
}

void w_Float2_set_x()
{
    sm::vec2* v2 = (sm::vec2*)ves_toforeign(0);
    v2->x = (float)ves_tonumber(1);
}

void w_Float2_get_x()
{
    sm::vec2* v2 = (sm::vec2*)ves_toforeign(0);
    ves_set_number(0, static_cast<double>(v2->x));
}

void w_Float2_set_y()
{
    sm::vec2* v2 = (sm::vec2*)ves_toforeign(0);
    v2->y = (float)ves_tonumber(1);
}

void w_Float2_get_y()
{
    sm::vec2* v2 = (sm::vec2*)ves_toforeign(0);
    ves_set_number(0, static_cast<double>(v2->y));
}

void w_Float2_transform()
{
    sm::vec2* v2 = (sm::vec2*)ves_toforeign(0);
    sm::Matrix2D* mt = (sm::Matrix2D*)ves_toforeign(1);
    *v2 = *mt * *v2;
}

void w_Float3_allocate()
{
    sm::vec3* v3 = (sm::vec3*)ves_set_newforeign(0, 0, sizeof(sm::vec3));
    v3->x = 0;
    v3->y = 0;
    v3->z = 0;
}

int w_Float3_finalize(void* data)
{
    return sizeof(sm::vec3);
}

void w_Float3_set_x()
{
    sm::vec3* v3 = (sm::vec3*)ves_toforeign(0);
    v3->x = (float)ves_tonumber(1);
}

void w_Float3_get_x()
{
    sm::vec3* v3 = (sm::vec3*)ves_toforeign(0);
    ves_set_number(0, static_cast<double>(v3->x));
}

void w_Float3_set_y()
{
    sm::vec3* v3 = (sm::vec3*)ves_toforeign(0);
    v3->y = (float)ves_tonumber(1);
}

void w_Float3_get_y()
{
    sm::vec3* v3 = (sm::vec3*)ves_toforeign(0);
    ves_set_number(0, static_cast<double>(v3->y));
}

void w_Float3_set_z()
{
    sm::vec3* v3 = (sm::vec3*)ves_toforeign(0);
    v3->z = (float)ves_tonumber(1);
}

void w_Float3_get_z()
{
    sm::vec3* v3 = (sm::vec3*)ves_toforeign(0);
    ves_set_number(0, static_cast<double>(v3->z));
}

void w_Float3_transform()
{
    sm::vec3* v3 = (sm::vec3*)ves_toforeign(0);
    sm::mat4* mt = (sm::mat4*)ves_toforeign(1);
    *v3 = *mt * *v3;
}

void w_Float4_allocate()
{
    sm::vec4* v4 = (sm::vec4*)ves_set_newforeign(0, 0, sizeof(sm::vec4));
    v4->x = 0;
    v4->y = 0;
    v4->z = 0;
    v4->w = 0;
}

int w_Float4_finalize(void* data)
{
    return sizeof(sm::vec4);
}

void w_Float4_set_x()
{
    sm::vec4* v4 = (sm::vec4*)ves_toforeign(0);
    v4->x = (float)ves_tonumber(1);
}

void w_Float4_get_x()
{
    sm::vec4* v4 = (sm::vec4*)ves_toforeign(0);
    ves_set_number(0, static_cast<double>(v4->x));
}

void w_Float4_set_y()
{
    sm::vec4* v4 = (sm::vec4*)ves_toforeign(0);
    v4->y = (float)ves_tonumber(1);
}

void w_Float4_get_y()
{
    sm::vec4* v4 = (sm::vec4*)ves_toforeign(0);
    ves_set_number(0, static_cast<double>(v4->y));
}

void w_Float4_set_z()
{
    sm::vec4* v4 = (sm::vec4*)ves_toforeign(0);
    v4->z = (float)ves_tonumber(1);
}

void w_Float4_get_z()
{
    sm::vec4* v4 = (sm::vec4*)ves_toforeign(0);
    ves_set_number(0, static_cast<double>(v4->z));
}

void w_Float4_set_w()
{
    sm::vec4* v4 = (sm::vec4*)ves_toforeign(0);
    v4->w = (float)ves_tonumber(1);
}

void w_Float4_get_w()
{
    sm::vec4* v4 = (sm::vec4*)ves_toforeign(0);
    ves_set_number(0, static_cast<double>(v4->w));
}

void w_Matrix2D_transform()
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

void w_Matrix44_clone()
{
    sm::mat4* src = (sm::mat4*)ves_toforeign(0);
    sm::mat4* dst = (sm::mat4*)ves_set_newforeign(0, 0, sizeof(sm::mat4));
    *dst = *src;
}

void w_Matrix44_translate()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    const float x = (float)ves_tonumber(1);
    const float y = (float)ves_tonumber(2);
    const float z = (float)ves_tonumber(3);
    mt->Translate(x, y, z);
}

void w_Matrix44_rotate_axis()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    const float angle = (float)ves_tonumber(1);
    const float x = (float)ves_tonumber(2);
    const float y = (float)ves_tonumber(3);
    const float z = (float)ves_tonumber(4);
    sm::vec3 axis(x, y, z);
    axis.Normalize();
    *mt = mt->RotatedAxis(axis, angle);
}

void w_Matrix44_scale()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    const float x = (float)ves_tonumber(1);
    const float y = (float)ves_tonumber(2);
    const float z = (float)ves_tonumber(3);
    mt->Scale(x, y, z);
}

void w_Matrix44_perspective()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    const float fovy   = (float)ves_tonumber(1);
    float aspect = (float)ves_tonumber(2);
    if (aspect == 0) 
    {
        auto g = tt::Graphics::Instance();
        if (g->GetHeight() == 0) {
            aspect = 1;
        } else {
            aspect = g->GetWidth() / g->GetHeight();
        }
    }
    const float znear  = (float)ves_tonumber(3);
    const float zfar   = (float)ves_tonumber(4);
    *mt = sm::mat4::Perspective(fovy, aspect, znear, zfar);
}

void w_Matrix44_orthographic()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    const float left   = (float)ves_tonumber(1);
    const float right  = (float)ves_tonumber(2);
    const float bottom = (float)ves_tonumber(3);
    const float top    = (float)ves_tonumber(4);
    const float near   = (float)ves_tonumber(5);
    const float far    = (float)ves_tonumber(6);

    float w = 0, h = 0;
    if (left == 0 && right == 0 && bottom == 0 && top == 0) {
        auto g = tt::Graphics::Instance();
        w = g->GetWidth();
        h = g->GetHeight();
    } else {
        w = right - left;
        h = top - bottom;
    }
    const float hw = w * 0.5f;
    const float hh = h * 0.5f;
    *mt = sm::mat4::Orthographic(-hw, hw, -hh, hh, near, far);
}

void w_Matrix44_lookat()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);

    sm::vec3 eye, center, up;

    ves_getfield(1, "x");
    eye.x = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(1, "y");
    eye.y = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(1, "z");
    eye.z = (float)ves_tonumber(-1);
    ves_pop(1);

    ves_getfield(2, "x");
    center.x = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(2, "y");
    center.y = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(2, "z");
    center.z = (float)ves_tonumber(-1);
    ves_pop(1);
    
    ves_getfield(3, "x");
    up.x = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(3, "y");
    up.y = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_getfield(3, "z");
    up.z = (float)ves_tonumber(-1);
    ves_pop(1);

    *mt = sm::mat4::LookAt(eye, center, up);
}

void w_Matrix44_from_rotate_mat()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);

    sm::mat4* src_mt = (sm::mat4*)ves_toforeign(1);
    sm::mat3 rot(*src_mt);
    *mt = sm::mat4(rot);    
}

void w_Matrix44_from_quaternion()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);

    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);
    float z = (float)ves_tonumber(3);
    float w = (float)ves_tonumber(4);

    *mt = sm::mat4(sm::Quaternion(x, y, z, w));
}

void w_Matrix44_transform_mat2d()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    sm::Matrix2D* trans = (sm::Matrix2D*)ves_toforeign(1);
    *mt = sm::mat4(*trans) * *mt;
}

void w_Matrix44_transform_mat4()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    sm::mat4* trans = (sm::mat4*)ves_toforeign(1);
    *mt = *trans * *mt;
}

void w_Matrix44_get_scale()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);

    ves_pop(1);
    ves_newlist(3);

    ves_pushnumber(mt->c[0][0]);
    ves_seti(-2, 0);
    ves_pop(1);
    ves_pushnumber(mt->c[1][1]);
    ves_seti(-2, 1);
    ves_pop(1);
    ves_pushnumber(mt->c[2][2]);
    ves_seti(-2, 2);
    ves_pop(1);
}

void w_Matrix44_inverse()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    *mt = mt->Inverted();
}

void w_Maths_is_convex_intersect_convex()
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
    //if (strcmp(signature, "Float2.x=(_)") == 0) return w_Float2_set_x;
    //if (strcmp(signature, "Float2.x()") == 0) return w_Float2_get_x;
    if (strcmp(signature, "Float2.set_x(_)") == 0) return w_Float2_set_x;
    if (strcmp(signature, "Float2.x()") == 0) return w_Float2_get_x;
    if (strcmp(signature, "Float2.set_y(_)") == 0) return w_Float2_set_y;
    if (strcmp(signature, "Float2.y()") == 0) return w_Float2_get_y;
    if (strcmp(signature, "Float2.transform(_)") == 0) return w_Float2_transform;

    if (strcmp(signature, "Float3.set_x(_)") == 0) return w_Float3_set_x;
    if (strcmp(signature, "Float3.x()") == 0) return w_Float3_get_x;
    if (strcmp(signature, "Float3.set_y(_)") == 0) return w_Float3_set_y;
    if (strcmp(signature, "Float3.y()") == 0) return w_Float3_get_y;
    if (strcmp(signature, "Float3.set_z(_)") == 0) return w_Float3_set_z;
    if (strcmp(signature, "Float3.z()") == 0) return w_Float3_get_z;
    if (strcmp(signature, "Float3.transform(_)") == 0) return w_Float3_transform;

    if (strcmp(signature, "Float4.set_x(_)") == 0) return w_Float4_set_x;
    if (strcmp(signature, "Float4.x()") == 0) return w_Float4_get_x;
    if (strcmp(signature, "Float4.set_y(_)") == 0) return w_Float4_set_y;
    if (strcmp(signature, "Float4.y()") == 0) return w_Float4_get_y;
    if (strcmp(signature, "Float4.set_z(_)") == 0) return w_Float4_set_z;
    if (strcmp(signature, "Float4.z()") == 0) return w_Float4_get_z;
    if (strcmp(signature, "Float4.set_w(_)") == 0) return w_Float4_set_w;
    if (strcmp(signature, "Float4.w()") == 0) return w_Float4_get_w;

    if (strcmp(signature, "Matrix2D.transform(_,_,_,_,_,_,_,_,_)") == 0) return w_Matrix2D_transform;

    if (strcmp(signature, "Matrix44.clone()") == 0) return w_Matrix44_clone;
    if (strcmp(signature, "Matrix44.translate(_,_,_)") == 0) return w_Matrix44_translate;
    if (strcmp(signature, "Matrix44.rotate_axis(_,_,_,_)") == 0) return w_Matrix44_rotate_axis;
    if (strcmp(signature, "Matrix44.scale(_,_,_)") == 0) return w_Matrix44_scale;
    if (strcmp(signature, "Matrix44.perspective(_,_,_,_)") == 0) return w_Matrix44_perspective;
    if (strcmp(signature, "Matrix44.orthographic(_,_,_,_,_,_)") == 0) return w_Matrix44_orthographic;
    if (strcmp(signature, "Matrix44.lookat(_,_,_)") == 0) return w_Matrix44_lookat;
    if (strcmp(signature, "Matrix44.from_rotate_mat(_)") == 0) return w_Matrix44_from_rotate_mat;
    if (strcmp(signature, "Matrix44.from_quaternion(_,_,_,_)") == 0) return w_Matrix44_from_quaternion;
    if (strcmp(signature, "Matrix44.transform_mat2d(_)") == 0) return w_Matrix44_transform_mat2d;
    if (strcmp(signature, "Matrix44.transform_mat4(_)") == 0) return w_Matrix44_transform_mat4;
    if (strcmp(signature, "Matrix44.get_scale()") == 0) return w_Matrix44_get_scale;
    if (strcmp(signature, "Matrix44.inverse()") == 0) return w_Matrix44_inverse;

    if (strcmp(signature, "static Maths.is_convex_intersect_convex(_,_)") == 0) return w_Maths_is_convex_intersect_convex;

	return nullptr;
}

void MathsBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "Float2") == 0)
    {
        methods->allocate = w_Float2_allocate;
        methods->finalize = w_Float2_finalize;
        return;
    }

    if (strcmp(class_name, "Float3") == 0)
    {
        methods->allocate = w_Float3_allocate;
        methods->finalize = w_Float3_finalize;
        return;
    }

    if (strcmp(class_name, "Float4") == 0)
    {
        methods->allocate = w_Float4_allocate;
        methods->finalize = w_Float4_finalize;
        return;
    }

    if (strcmp(class_name, "Matrix2D") == 0)
    {
        methods->allocate = w_Matrix2D_allocate;
        methods->finalize = w_Matrix2D_finalize;
        return;
    }

    if (strcmp(class_name, "Matrix44") == 0)
    {
        methods->allocate = w_Matrix44_allocate;
        methods->finalize = w_Matrix44_finalize;
        return;
    }
}

}