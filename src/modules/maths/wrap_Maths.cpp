#include "modules/maths/wrap_Maths.h"
#include "modules/script/TransHelper.h"
#include "modules/graphics/Graphics.h"

#include <SM_Matrix2D.h>
#include <SM_Test.h>
#include <SM_Matrix.h>
#include <SM_Plane.h>
#include <SM_Calc.h>
#include <SM_Cube.h>

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

void w_Matrix2D_transform_mat()
{
    sm::Matrix2D* mt = (sm::Matrix2D*)ves_toforeign(0);
    sm::Matrix2D* trans = (sm::Matrix2D*)ves_toforeign(1);
    *mt = *mt * *trans;
}

void w_Matrix2D_decompose()
{
    sm::Matrix2D* mt = (sm::Matrix2D*)ves_toforeign(0);

    sm::vec2 s, t;
    float r = 0.0f;
    mt->Decompose(s, r, t);

    tt::return_list(std::vector<float>{ s.x, s.y, r, t.x, t.y });
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

void w_Matrix44_rotate()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    const float x = (float)ves_tonumber(1);
    const float y = (float)ves_tonumber(2);
    const float z = (float)ves_tonumber(3);
    mt->Rotate(x, y, z);
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

void w_Matrix44_skew()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    const float x = (float)ves_tonumber(1);
    const float y = (float)ves_tonumber(2);
    const float z = (float)ves_tonumber(3);

    *mt = sm::mat4::SkewY(x, z);
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
    float left   = (float)ves_tonumber(1);
    float right  = (float)ves_tonumber(2);
    float bottom = (float)ves_tonumber(3);
    float top    = (float)ves_tonumber(4);
    float near   = (float)ves_tonumber(5);
    float far    = (float)ves_tonumber(6);

    if (left == 0 && right == 0 && bottom == 0 && top == 0) 
    {
        auto g = tt::Graphics::Instance();
        const float hw = g->GetWidth() * 0.5f;
        const float hh = g->GetHeight() * 0.5f;

        left   = -hw;
        right  =  hw;
        bottom = -hh;
        top    =  hh;
    }

    *mt = sm::mat4::Orthographic(left, right, bottom, top, near, far);
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

void w_Matrix44_viewport()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    float width  = (float)ves_tonumber(1);
    float height = (float)ves_tonumber(2);
    float near   = (float)ves_tonumber(3);
    float far    = (float)ves_tonumber(4);

    float hw = width * 0.5f;
    float hh = height * 0.5f;
    float hd = (far - near) * 0.5f;

    const float m[] = { 
        hw,   0.0f, 0.0f, 0.0f,
        0.0f, hh,   0.0f, 0.0f,
        0.0f, 0.0f, hd,   0.0f,
        0.0f, 0.0f, near + hd, 1.0f
    };

    memcpy(mt->x, m, sizeof(m));
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

void w_Matrix44_from_vectors()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);

    auto f = tt::list_to_vec3(1);
    auto t = tt::list_to_vec3(2);

    auto quat = sm::Quaternion::CreateFromVectors(f, t);
    *mt = sm::mat4(quat);
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
    tt::return_list(std::vector<float>{ mt->c[0][0], mt->c[1][1], mt->c[2][2] });
}

void w_Matrix44_inverse()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);
    *mt = mt->Inverted();
}

void w_Matrix44_decompose()
{
    sm::mat4* mt = (sm::mat4*)ves_toforeign(0);

    sm::vec3 s, r, t;
    mt->Decompose(t, r, s);

    tt::return_list(std::vector<float>{ s.x, s.y, s.z, r.x, r.y, r.z, t.x, t.y, t.z });
}

void w_Plane_allocate()
{
    sm::Plane* plane = (sm::Plane*)ves_set_newforeign(0, 0, sizeof(sm::Plane));

    int num = ves_argnum();
    if (num == 4)
    {
        auto p0 = tt::list_to_vec3(1);
        auto p1 = tt::list_to_vec3(2);
        auto p2 = tt::list_to_vec3(3);

        plane->Build(p0, p1, p2);
    }
    else if (num == 3)
    {
        auto ori = tt::list_to_vec3(1);
        auto dir = tt::list_to_vec3(2);

        plane->Build(dir, ori);
    }
}

int w_Plane_finalize(void* data)
{
    return sizeof(sm::Plane);
}

void w_Plane_clone()
{
    sm::Plane* src = (sm::Plane*)ves_toforeign(0);
    sm::Plane* dst = (sm::Plane*)ves_set_newforeign(0, 0, sizeof(sm::Plane));
    *dst = *src;
}

void w_Plane_transform()
{
    sm::Plane* p = (sm::Plane*)ves_toforeign(0);
    sm::mat4* mt = (sm::mat4*)ves_toforeign(1);

    auto& norm = p->normal;

    sm::vec3 pos0;
    if (norm.x != 0)
    {
        pos0.x = -p->dist / norm.x;
        pos0.y = 0;
        pos0.z = 0;
    }
    else if (norm.y != 0)
    {
        pos0.x = 0;
        pos0.y = -p->dist / norm.y;
        pos0.z = 0;
    }
    else if (norm.z != 0)
    {
        pos0.x = 0;
        pos0.y = 0;
        pos0.z = -p->dist / norm.z;
    }

    auto pos1 = pos0 + norm;

    pos0 = *mt * pos0;
    pos1 = *mt * pos1;
    p->Build(pos1 - pos0, pos0);
}

void w_Cube_allocate()
{
    (sm::cube*)ves_set_newforeign(0, 0, sizeof(sm::cube));
}

int w_Cube_finalize(void* data)
{
    return sizeof(sm::cube);
}

void w_Cube_get_center()
{
    sm::cube* cube = (sm::cube*)ves_toforeign(0);
    auto p = cube->Center();
    tt::return_list(std::vector<float>{ p.x, p.y, p.z });
}

void w_Cube_get_size()
{
    sm::cube* cube = (sm::cube*)ves_toforeign(0);
    auto sz = cube->Size();
    tt::return_list(std::vector<float>{ sz.x, sz.y, sz.z });
}

void w_Maths_is_convex_intersect_convex()
{
    auto c0 = tt::list_to_vec2_array(1);
    auto c1 = tt::list_to_vec2_array(2);
    bool ret = sm::is_convex_intersect_convex(c0, c1);
    ves_set_boolean(0, ret);
}

void w_Maths_get_line_intersect_line()
{
    auto l0 = tt::list_to_vec2_array(1);
    auto l1 = tt::list_to_vec2_array(2);

    bool is_seg = ves_toboolean(3);

    sm::vec2 cross;
    bool succ = false;
    if (is_seg) {
        succ = sm::intersect_segment_segment(l0[0], l0[1], l1[0], l1[1], &cross);
    } else {
        succ = sm::intersect_line_line(l0[0], l0[1], l1[0], l1[1], &cross);
    }
    if (succ) {
        tt::return_list(std::vector<float>{ cross.x, cross.y });
    } else {
        ves_set_nil(0);
    }
}

void w_Maths_calc_rot_mat()
{
    auto f = tt::list_to_vec3(1);
    auto t = tt::list_to_vec3(2);

    auto q = sm::Quaternion::CreateFromVectors(f, t);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("maths", "Matrix44");
    sm::mat4* mat = (sm::mat4*)ves_set_newforeign(0, 1, sizeof(sm::mat4));
    *mat = sm::mat4(q);
    ves_pop(1);
}

void w_Maths_calc_angle()
{
    auto c = tt::list_to_vec3(1);
    auto s = tt::list_to_vec3(2);
    auto e = tt::list_to_vec3(3);
    
    auto angle = sm::get_angle(c, s, e);
    ves_set_number(0, angle);
}

void w_Maths_is_pos_above_plane()
{
    auto pos = tt::list_to_vec3(1);
    auto plane = (sm::Plane*)ves_toforeign(2);
    bool ret = plane->GetDistance(pos) > 0;
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
    if (strcmp(signature, "Matrix2D.transform_mat(_)") == 0) return w_Matrix2D_transform_mat;
    if (strcmp(signature, "Matrix2D.decompose()") == 0) return w_Matrix2D_decompose;

    if (strcmp(signature, "Matrix44.clone()") == 0) return w_Matrix44_clone;
    if (strcmp(signature, "Matrix44.translate(_,_,_)") == 0) return w_Matrix44_translate;
    if (strcmp(signature, "Matrix44.rotate(_,_,_)") == 0) return w_Matrix44_rotate;
    if (strcmp(signature, "Matrix44.rotate_axis(_,_,_,_)") == 0) return w_Matrix44_rotate_axis;
    if (strcmp(signature, "Matrix44.scale(_,_,_)") == 0) return w_Matrix44_scale;
    if (strcmp(signature, "Matrix44.skew(_,_,_)") == 0) return w_Matrix44_skew;
    if (strcmp(signature, "Matrix44.perspective(_,_,_,_)") == 0) return w_Matrix44_perspective;
    if (strcmp(signature, "Matrix44.orthographic(_,_,_,_,_,_)") == 0) return w_Matrix44_orthographic;
    if (strcmp(signature, "Matrix44.lookat(_,_,_)") == 0) return w_Matrix44_lookat;
    if (strcmp(signature, "Matrix44.viewport(_,_,_,_)") == 0) return w_Matrix44_viewport;
    if (strcmp(signature, "Matrix44.from_rotate_mat(_)") == 0) return w_Matrix44_from_rotate_mat;
    if (strcmp(signature, "Matrix44.from_quaternion(_,_,_,_)") == 0) return w_Matrix44_from_quaternion;
    if (strcmp(signature, "Matrix44.from_vectors(_,_)") == 0) return w_Matrix44_from_vectors;
    if (strcmp(signature, "Matrix44.transform_mat2d(_)") == 0) return w_Matrix44_transform_mat2d;
    if (strcmp(signature, "Matrix44.transform_mat4(_)") == 0) return w_Matrix44_transform_mat4;
    if (strcmp(signature, "Matrix44.get_scale()") == 0) return w_Matrix44_get_scale;
    if (strcmp(signature, "Matrix44.inverse()") == 0) return w_Matrix44_inverse;
    if (strcmp(signature, "Matrix44.decompose()") == 0) return w_Matrix44_decompose;

    if (strcmp(signature, "Plane.clone()") == 0) return w_Plane_clone;
    if (strcmp(signature, "Plane.transform(_)") == 0) return w_Plane_transform;

    if (strcmp(signature, "Cube.get_center()") == 0) return w_Cube_get_center;
    if (strcmp(signature, "Cube.get_size()") == 0) return w_Cube_get_size;

    if (strcmp(signature, "static Maths.is_convex_intersect_convex(_,_)") == 0) return w_Maths_is_convex_intersect_convex;
    if (strcmp(signature, "static Maths.get_line_intersect_line(_,_,_)") == 0) return w_Maths_get_line_intersect_line;
    if (strcmp(signature, "static Maths.calc_rot_mat(_,_)") == 0) return w_Maths_calc_rot_mat;
    if (strcmp(signature, "static Maths.calc_angle(_,_,_)") == 0) return w_Maths_calc_angle;
    if (strcmp(signature, "static Maths.is_pos_above_plane(_,_)") == 0) return w_Maths_is_pos_above_plane;

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

    if (strcmp(class_name, "Plane") == 0)
    {
        methods->allocate = w_Plane_allocate;
        methods->finalize = w_Plane_finalize;
        return;
    }

    if (strcmp(class_name, "Cube") == 0)
    {
        methods->allocate = w_Cube_allocate;
        methods->finalize = w_Cube_finalize;
        return;
    }
}

}