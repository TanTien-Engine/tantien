#include "wrap_OM.h"
#include "modules/script/TransHelper.h"
#include "modules/render/Render.h"

#include <brepom/Label.h>
#include <brepom/TopoShape.h>
#include <brepom/TopoVertex.h>
#include <brepom/TopoEdge.h>
#include <brepom/TopoLoop.h>
#include <brepom/TopoFace.h>
#include <brepom/TopoShell.h>
#include <brepom/TopoBody.h>
#include <brepom/BRepBuilder.h>
#include <brepom/RenderBuilder.h>
#include <brepom/AttrNamedShape.h>
#include <brepom/AttrRenderObj.h>
#include <brepom/AttrColor.h>
#include <brepom/TopoAdapter.h>
#include <brepom/LabelBuilder.h>
#include <brepom/PrimBuilder.h>
#include <polymesh3/Polytope.h>

#include <memory>
#include <iterator>

#define DELTA_UPDATES

namespace
{

void w_BRepTools_poly2shape()
{
    auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;

    std::vector<sm::vec3> points;
    auto& src_points = poly->Points();
    points.reserve(src_points.size());
    for (auto& p : src_points) {
        points.push_back(p->pos);
    }

    std::vector<std::vector<uint32_t>> faces;
    auto& src_faces = poly->Faces();
    faces.reserve(src_faces.size());
    for (auto& s_f : src_faces) 
    {
        std::vector<uint32_t> f;
        f.reserve(s_f->border.size());
        for (auto i : s_f->border) {
            f.push_back(static_cast<uint32_t>(i));
        }

        faces.push_back(f);
    }

    auto shape = brepom::BRepBuilder::BuildShell(points, faces);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("om", "TopoShell");
    auto proxy = (tt::Proxy<brepom::TopoShell>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<brepom::TopoShell>));
    proxy->obj = shape;
    ves_pop(1);
}

void w_BRepTools_shape2vao()
{
    auto shape = ((tt::Proxy<brepom::TopoShape>*)ves_toforeign(1))->obj;
    auto color = tt::map_to_vec3(2);

    auto dev = tt::Render::Instance()->Device();
    auto vao = brepom::RenderBuilder::BuildVAO(*dev, shape, color);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("render", "VertexArray");
    auto proxy = (tt::Proxy<ur::VertexArray>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<ur::VertexArray>));
    proxy->obj = vao;
    ves_pop(1);
}

void w_BRepAlgos_clip()
{
    auto shape = ((tt::Proxy<brepom::TopoShape>*)ves_toforeign(1))->obj;
    sm::Plane* plane = (sm::Plane*)ves_toforeign(2);

    auto keep_str = ves_tostring(3);
    auto keep = he::Polyhedron::KeepType::KeepAbove;
    if (strcmp(keep_str, "above") == 0) {
        keep = he::Polyhedron::KeepType::KeepAbove;
    } else if (strcmp(keep_str, "below") == 0) {
        keep = he::Polyhedron::KeepType::KeepBelow;
    } else if (strcmp(keep_str, "all") == 0) {
        keep = he::Polyhedron::KeepType::KeepAll;
    }

    auto seam_face = ves_toboolean(4);

    auto topo = brepom::TopoAdapter::BRep2Topo(shape);
    if (!topo->Clip(*plane, keep, seam_face)) {
        ves_set_nil(0);
        return;
    }

#ifdef DELTA_UPDATES
    brepom::TopoAdapter::Topo2BRep(topo, shape);
#else
    shape = brepom::TopoAdapter::Topo2BRep(topo);
#endif // DELTA_UPDATES

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("om", "TopoShape");
    auto proxy = (tt::Proxy<brepom::TopoShape>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<brepom::TopoShape>));
    proxy->obj = shape;
    ves_pop(1);
}

void w_PrimBuilder_box()
{
    auto center = tt::map_to_vec3(1);
    auto size = tt::map_to_vec3(2);
    auto shape = brepom::PrimBuilder::Box(center, size);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("om", "TopoShape");
    auto proxy = (tt::Proxy<brepom::TopoShape>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<brepom::TopoShape>));
    proxy->obj = shape;
    ves_pop(1);
}

void w_Label_allocate()
{
    auto proxy = (tt::Proxy<brepom::Label>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<brepom::Label>));
    proxy->obj = std::make_shared<brepom::Label>();
}

int w_Label_finalize(void* data)
{
    auto proxy = (tt::Proxy<brepom::Label>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<brepom::Label>);
}

void w_Label_set_shape()
{
    auto label = ((tt::Proxy<brepom::Label>*)ves_toforeign(0))->obj;
    auto shape = ((tt::Proxy<brepom::TopoShape>*)ves_toforeign(1))->obj;
    brepom::LabelBuilder::BuildFromShape(label, shape);
}

void w_Label_get_shape()
{
    auto label = ((tt::Proxy<brepom::Label>*)ves_toforeign(0))->obj;
    if (!label->HasComponent<brepom::AttrNamedShape>()) {
        ves_set_nil(0);
        return;
    }

    auto& named_shape = label->GetComponent<brepom::AttrNamedShape>();
    auto shape = named_shape.GetDst();

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("om", "TopoShape");
    auto proxy = (tt::Proxy<brepom::TopoShape>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<brepom::TopoShape>));
    proxy->obj = shape;
    ves_pop(1);
}

void w_Label_set_render_obj()
{
    auto label = ((tt::Proxy<brepom::Label>*)ves_toforeign(0))->obj;
    auto va = ((tt::Proxy<ur::VertexArray>*)ves_toforeign(1))->obj;
    if (label->HasComponent<brepom::AttrRenderObj>()) {
        label->GetComponent<brepom::AttrRenderObj>().SetVA(va);
    } else {
        label->AddComponent<brepom::AttrRenderObj>(va);
    }
}

void w_Label_get_render_obj()
{
    auto label = ((tt::Proxy<brepom::Label>*)ves_toforeign(0))->obj;
    if (!label->HasComponent<brepom::AttrRenderObj>()) {
        ves_set_nil(0);
        return;
    }

    auto& render_obj = label->GetComponent<brepom::AttrRenderObj>();
    auto va = render_obj.GetVA();

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("render", "VertexArray");
    auto proxy = (tt::Proxy<ur::VertexArray>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<ur::VertexArray>));
    proxy->obj = va;
    ves_pop(1);
}

void w_Label_set_color()
{
    auto label = ((tt::Proxy<brepom::Label>*)ves_toforeign(0))->obj;
    sm::vec3 rgb = tt::map_to_vec3(1);
    if (label->HasComponent<brepom::AttrColor>()) {
        label->GetComponent<brepom::AttrColor>().SetColor(rgb);
    } else {
        label->AddComponent<brepom::AttrColor>(rgb);
    }
}

bool get_label_color(const brepom::Label* label, sm::vec3& color)
{
    if (!label) {
        return false;
    }

    if (label->HasComponent<brepom::AttrColor>())
    {
        color = label->GetComponent<brepom::AttrColor>().GetColor();
        return true;
    }
    else
    {
        auto parent = label->GetParent();
        if (parent) {
            return get_label_color(parent, color);
        }
    }
    return false;
}

void w_Label_get_color()
{
    auto label = ((tt::Proxy<brepom::Label>*)ves_toforeign(0))->obj;

    sm::vec3 color;
    if (get_label_color(label.get(), color)) {
        tt::return_vec(color);
    } else {
        tt::return_vec(sm::vec3(1, 1, 1));
    }
}

void w_Label_get_children()
{
    auto label = ((tt::Proxy<brepom::Label>*)ves_toforeign(0))->obj;
    auto& children = label->GetAllChildren();

    ves_pop(ves_argnum());

    const int num = (int)(children.size());
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        ves_pushnil();
        ves_import_class("om", "Label");
        auto proxy = (tt::Proxy<brepom::Label>*)ves_set_newforeign(1, 2, sizeof(tt::Proxy<brepom::Label>));
        proxy->obj = children[i];
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Label_build_vao()
{
    auto label = ((tt::Proxy<brepom::Label>*)ves_toforeign(0))->obj;

}

void w_TopoVertex_allocate()
{
    auto proxy = (tt::Proxy<brepom::TopoVertex>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<brepom::TopoVertex>));
    proxy->obj = std::make_shared<brepom::TopoVertex>();
}

int w_TopoVertex_finalize(void* data)
{
    auto proxy = (tt::Proxy<brepom::TopoVertex>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<brepom::TopoVertex>);
}

void w_TopoEdge_allocate()
{
    auto proxy = (tt::Proxy<brepom::TopoEdge>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<brepom::TopoEdge>));
    proxy->obj = std::make_shared<brepom::TopoEdge>();
}

int w_TopoEdge_finalize(void* data)
{
    auto proxy = (tt::Proxy<brepom::TopoEdge>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<brepom::TopoEdge>);
}

void w_TopoLoop_allocate()
{
    auto proxy = (tt::Proxy<brepom::TopoLoop>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<brepom::TopoLoop>));
    proxy->obj = std::make_shared<brepom::TopoLoop>();
}

int w_TopoLoop_finalize(void* data)
{
    auto proxy = (tt::Proxy<brepom::TopoLoop>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<brepom::TopoLoop>);
}

void w_TopoFace_allocate()
{
    auto proxy = (tt::Proxy<brepom::TopoFace>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<brepom::TopoFace>));
    proxy->obj = std::make_shared<brepom::TopoFace>();
}

int w_TopoFace_finalize(void* data)
{
    auto proxy = (tt::Proxy<brepom::TopoFace>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<brepom::TopoFace>);
}

void w_TopoShell_allocate()
{
    auto proxy = (tt::Proxy<brepom::TopoShell>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<brepom::TopoShell>));
    proxy->obj = std::make_shared<brepom::TopoShell>();
}

int w_TopoShell_finalize(void* data)
{
    auto proxy = (tt::Proxy<brepom::TopoShell>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<brepom::TopoShell>);
}

void w_TopoBody_allocate()
{
    auto proxy = (tt::Proxy<brepom::TopoBody>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<brepom::TopoBody>));
    proxy->obj = std::make_shared<brepom::TopoBody>();
}

int w_TopoBody_finalize(void* data)
{
    auto proxy = (tt::Proxy<brepom::TopoBody>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<brepom::TopoBody>);
}

}

namespace tt
{

VesselForeignMethodFn OmBindMethod(const char* signature)
{
    if (strcmp(signature, "static BRepTools.poly2shape(_)") == 0) return w_BRepTools_poly2shape;
    if (strcmp(signature, "static BRepTools.shape2vao(_,_)") == 0) return w_BRepTools_shape2vao;

    if (strcmp(signature, "static BRepAlgos.clip(_,_,_,_)") == 0) return w_BRepAlgos_clip;

    if (strcmp(signature, "static PrimBuilder.box(_,_)") == 0) return w_PrimBuilder_box;

    if (strcmp(signature, "Label.set_shape(_)") == 0) return w_Label_set_shape;
    if (strcmp(signature, "Label.get_shape()") == 0) return w_Label_get_shape;

    if (strcmp(signature, "Label.set_render_obj(_)") == 0) return w_Label_set_render_obj;
    if (strcmp(signature, "Label.get_render_obj()") == 0) return w_Label_get_render_obj;

    if (strcmp(signature, "Label.set_color(_)") == 0) return w_Label_set_color;
    if (strcmp(signature, "Label.get_color()") == 0) return w_Label_get_color;

    if (strcmp(signature, "Label.get_children()") == 0) return w_Label_get_children;

    if (strcmp(signature, "Label.build_vao()") == 0) return w_Label_build_vao;

    if (strcmp(signature, "Label.build_vao()") == 0) return w_Label_build_vao;

	return nullptr;
}

void OmBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "Label") == 0)
    {
        methods->allocate = w_Label_allocate;
        methods->finalize = w_Label_finalize;
        return;
    }

    if (strcmp(class_name, "TopoVertex") == 0)
    {
        methods->allocate = w_TopoVertex_allocate;
        methods->finalize = w_TopoVertex_finalize;
        return;
    }

    if (strcmp(class_name, "TopoEdge") == 0)
    {
        methods->allocate = w_TopoEdge_allocate;
        methods->finalize = w_TopoEdge_finalize;
        return;
    }

    if (strcmp(class_name, "TopoLoop") == 0)
    {
        methods->allocate = w_TopoLoop_allocate;
        methods->finalize = w_TopoLoop_finalize;
        return;
    }

    if (strcmp(class_name, "TopoFace") == 0)
    {
        methods->allocate = w_TopoFace_allocate;
        methods->finalize = w_TopoFace_finalize;
        return;
    }

    if (strcmp(class_name, "TopoShell") == 0)
    {
        methods->allocate = w_TopoShell_allocate;
        methods->finalize = w_TopoShell_finalize;
        return;
    }

    if (strcmp(class_name, "TopoBody") == 0)
    {
        methods->allocate = w_TopoBody_allocate;
        methods->finalize = w_TopoBody_finalize;
        return;
    }
}

}