#include "modules/regen/wrap_Regen.h"
#include "modules/regen/PolyDiff.h"

#include <wrapper/TransHelper.h>
#include <wrapper/Proxy.h>

#include <string>

namespace
{

void w_PolyDiff_allocate()
{
    auto proxy = (wrapper::Proxy<tt::PolyDiff>*)ves_set_newforeign(0, 0, sizeof(wrapper::Proxy<tt::PolyDiff>));
    proxy->obj = std::make_shared<tt::PolyDiff>();
}

int w_PolyDiff_finalize(void* data)
{
    auto proxy = (wrapper::Proxy<tt::PolyDiff>*)(data);
    proxy->~Proxy();
    return sizeof(wrapper::Proxy<tt::PolyDiff>);
}

void w_PolyDiff_build()
{
    auto diff = ((wrapper::Proxy<tt::PolyDiff>*)ves_toforeign(0))->obj;

    std::vector<std::shared_ptr<pm3::Polytope>> src, dst;
    wrapper::list_to_foreigns(1, src);
    wrapper::list_to_foreigns(2, dst);

    std::set<std::shared_ptr<pm3::Polytope>> src_set, dst_set;
    for (auto poly : src) {
        src_set.insert(poly);
    }
    for (auto poly : dst) {
        dst_set.insert(poly);
    }

    for (auto poly : src) {
        if (dst_set.find(poly) == dst_set.end()) {
            diff->AddDeleted(poly);
        }
    }

    for (auto poly : dst) {
        if (src_set.find(poly) == src_set.end()) {
            diff->AddGenerated(nullptr, poly);
        }
    }
}

}

namespace tt
{

VesselForeignMethodFn RegenBindMethod(const char* signature)
{
    if (strcmp(signature, "PolyDiff.build(_,_)") == 0) return w_PolyDiff_build;

    return nullptr;
}

void RegenBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "PolyDiff") == 0)
    {
        methods->allocate = w_PolyDiff_allocate;
        methods->finalize = w_PolyDiff_finalize;
        return;
    }
}

}