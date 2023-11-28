#include "wrap_Scene.h"
#include "modules/script/TransHelper.h"
#include "modules/scene/SceneTree.h"

#include <brepdb/RTree.h>

#include <queue>

#include <string.h>

namespace
{

void w_SceneTree_allocate()
{
    auto proxy = (tt::Proxy<tt::SceneTree>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<tt::SceneTree>));
    proxy->obj = std::make_shared<tt::SceneTree>();
}

int w_SceneTree_finalize(void* data)
{
    auto proxy = (tt::Proxy<tt::SceneTree>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<tt::SceneTree>);
}

void w_SceneTree_build_tree()
{
    auto stree = ((tt::Proxy<tt::SceneTree>*)ves_toforeign(0))->obj;
    auto rtree = ((tt::Proxy<brepdb::RTree>*)ves_toforeign(1))->obj;
    stree->Build(*rtree);
}

void w_SceneTree_get_all_vao()
{
    auto stree = ((tt::Proxy<tt::SceneTree>*)ves_toforeign(0))->obj;

    auto root = stree->GetRoot();
    if (!root) 
    {
        ves_set_nil(0);
        return;
    }

    std::vector<std::shared_ptr<ur::VertexArray>> vao_list;

    std::queue<std::shared_ptr<tt::SceneNode>> buf;
    buf.push(root);
    while (!buf.empty())
    {
        auto n = buf.front(); buf.pop();

        if (n->vao) {
            vao_list.push_back(n->vao);
        }

        for (auto child : n->children) {
            buf.push(child);
        }
    }

    ves_pop(ves_argnum());

    const int num = (int)(vao_list.size());
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        ves_pushnil();
        ves_import_class("render", "VertexArray");
        auto proxy = (tt::Proxy<ur::VertexArray>*)ves_set_newforeign(1, 2, sizeof(tt::Proxy<ur::VertexArray>));
        proxy->obj = vao_list[i];
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

}

namespace tt
{

VesselForeignMethodFn SceneBindMethod(const char* signature)
{
    if (strcmp(signature, "SceneTree.build(_)") == 0) return w_SceneTree_build_tree;
    if (strcmp(signature, "SceneTree.get_all_vao()") == 0) return w_SceneTree_get_all_vao;

    return nullptr;
}

void SceneBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "SceneTree") == 0)
    {
        methods->allocate = w_SceneTree_allocate;
        methods->finalize = w_SceneTree_finalize;
        return;
    }
}

}