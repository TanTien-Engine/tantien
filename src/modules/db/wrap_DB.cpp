#include "wrap_DB.h"
#include "BRepKey.h"
#include "RegionVisitor.h"
#include "PickVisitor.h"
#include "RTreeBuilder.h"
#include "DB.h"
#include "RTreeUpdate.h"
#include "BrepSerialize.h"
#include "modules/regen/PolyDiff.h"

#include <spatialdb/RTree.h>
#include <spatialdb/Region.h>
#include <spatialdb/Point.h>
#include <spatialdb/MemoryStorageManager.h>
#include <spatialdb/DiskStorageManager.h>
#include <spatialdb/ObjVisitor.h>
#include <polymesh3/Polytope.h>
#include <geoshape/Box.h>
#include <SM_Cube.h>
#include <wrapper/TransHelper.h>

#include <queue>

namespace
{

sm::cube region_to_cube(const spatialdb::Region& r)
{
    const auto l = r.GetLow();
    const auto h = r.GetHigh();

    sm::vec3 min, max;
    for (int i = 0; i < 3; ++i) {
        min.xyz[i] = static_cast<float>(l[i]);
    }
    for (int i = 0; i < 3; ++i) {
        max.xyz[i] = static_cast<float>(h[i]);
    }

    return sm::cube(min, max);
}

void return_regions(const std::vector<spatialdb::Region>& regions)
{
    ves_pop(ves_argnum());

    const int num = (int)(regions.size());
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        auto cube = region_to_cube(regions[i]);

        ves_pushnil();
        ves_import_class("geometry", "Box");
        auto proxy = (wrapper::Proxy<gs::Box>*)ves_set_newforeign(1, 2, sizeof(wrapper::Proxy<gs::Box>));
        proxy->obj = std::make_shared<gs::Box>(cube);
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_RTree_allocate()
{
    auto proxy = (wrapper::Proxy<spatialdb::RTree>*)ves_set_newforeign(0, 0, sizeof(wrapper::Proxy<spatialdb::RTree>));

    auto num = ves_argnum();
    if (num == 2)
    {
        auto sm = ((wrapper::Proxy<spatialdb::DiskStorageManager>*)ves_toforeign(1))->obj;
        proxy->obj = std::make_shared<spatialdb::RTree>(sm, false);
    }
    else
    {
//        auto sm = std::make_shared<spatialdb::MemoryStorageManager>();
        auto sm = std::make_shared<spatialdb::DiskStorageManager>("test_db");
        proxy->obj = std::make_shared<spatialdb::RTree>(sm, true);
    }
}

int w_RTree_finalize(void* data)
{
    auto proxy = (wrapper::Proxy<spatialdb::RTree>*)(data);
    proxy->~Proxy();
    return sizeof(wrapper::Proxy<spatialdb::RTree>);
}

void w_RTree_load_from_file()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    auto filepath = ves_tostring(1);
    if (!filepath) {
        return;
    }

    tt::RTreeBuilder::FromModeling(*rtree, filepath);
}

void w_RTree_insert()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    auto poly = ((wrapper::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;

    auto rkey = tt::RTreeUpdate::Insert(rtree, poly);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("db", "RKey");
    auto proxy = (wrapper::Proxy<tt::BRepKey>*)ves_set_newforeign(0, 1, sizeof(wrapper::Proxy<tt::BRepKey>));
    proxy->obj = rkey;
    ves_pop(1);
}

void w_RTree_insert_with_time()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    auto poly = ((wrapper::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;
    double time = ves_tonumber(2);

    uint8_t* data = nullptr;
    uint32_t length = 0;
    tt::BrepSerialize::BRepToByteArray(*poly, &data, length);

    spatialdb::id_type id = 0;

    spatialdb::Region aabb;
    auto& pts = poly->Points();
    for (auto& p : pts) 
    {
        auto src = p->pos.xyz;
        const double dst[4] = { src[0], src[1], src[2], time };
        aabb.Combine(spatialdb::Point(dst));
    }

    rtree->InsertData(length, data, aabb, id);
    delete[] data;

    ves_pop(ves_argnum());

    auto rkey = std::make_shared<tt::BRepKey>();
    rkey->r = aabb;
    rkey->id = id;

    ves_pushnil();
    ves_import_class("db", "RKey");
    auto proxy = (wrapper::Proxy<tt::BRepKey>*)ves_set_newforeign(0, 1, sizeof(wrapper::Proxy<tt::BRepKey>));
    proxy->obj = rkey;
    ves_pop(1);
}

void w_RTree_query()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    auto key = ((wrapper::Proxy<tt::BRepKey>*)ves_toforeign(1))->obj;
    
    auto visitor = std::make_unique<spatialdb::ObjVisitor>();
    rtree->IntersectsWithQuery(key->r, *visitor);

    std::vector<pm3::PolytopePtr> polys;

    auto& items = visitor->GetResults();
    for (auto item : items)
    {
        uint32_t len = 0;
        uint8_t* data = nullptr;
        item->GetData(len, &data);

        auto poly = tt::BrepSerialize::BRepFromByteArray(data, len);
        delete[] data;

        if (poly) {
            polys.push_back(poly);
        }
    }

    wrapper::return_foreign_list(polys, "geometry", "Polytope");
}

void w_RTree_delete()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    auto key = ((wrapper::Proxy<tt::BRepKey>*)ves_toforeign(1))->obj;

    if (key->id < 0)
    {
        auto visitor = std::make_unique<spatialdb::ObjVisitor>();
        rtree->IntersectsWithQuery(key->r, *visitor);

        auto& items = visitor->GetResults();
        for (auto item : items)
        {
            spatialdb::IShape* shape;
            item->GetShape(&shape);
            rtree->DeleteData(*shape, item->GetIdentifier());
            delete shape;
        }
    }
    else
    {
        rtree->DeleteData(key->r, key->id);
    }
}

void w_RTree_clear()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    tt::RTreeUpdate::Clear(rtree);
}

void w_RTree_rollforward()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    auto diff = ((wrapper::Proxy<tt::PolyDiff>*)ves_toforeign(1))->obj;
    tt::RTreeUpdate::RollForward(rtree, diff);
}

void w_RTree_rollback()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    auto diff = ((wrapper::Proxy<tt::PolyDiff>*)ves_toforeign(1))->obj;
    tt::RTreeUpdate::RollBack(rtree, diff);
}

void w_RTree_query_with_time()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    auto key = ((wrapper::Proxy<tt::BRepKey>*)ves_toforeign(1))->obj;

    double tmin = ves_tonumber(2);
    double tmax = ves_tonumber(3);
    (void)tmin;
    (void)tmax;

    // NOTE: the underlying spatialdb index is 3D (Region is double[DIMENSION],
    // DIMENSION == 3). Writing a 4th coordinate (the [3] time slot) overran the
    // array, and Point's constructor drops the 4th value anyway, so the time
    // bounds were never honoured. The out-of-bounds writes are removed; this
    // query degrades to a pure 3D spatial intersection. Re-add real time
    // filtering only after the index is made 4D.
    spatialdb::Region r = key->r;

    auto visitor = std::make_unique<spatialdb::ObjVisitor>();
    rtree->IntersectsWithQuery(r, *visitor);

    std::vector<pm3::PolytopePtr> polys;

    auto& items = visitor->GetResults();
    for (auto item : items)
    {
        uint32_t len = 0;
        uint8_t* data = nullptr;
        item->GetData(len, &data);

        auto poly = tt::BrepSerialize::BRepFromByteArray(data, len);
        delete[] data;

        if (poly) {
            polys.push_back(poly);
        }
    }

    wrapper::return_foreign_list(polys, "geometry", "Polytope");
}

void w_RTree_get_all_leaves()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;

    tt::RegionVisitor visitor;
    rtree->LevelTraversal(visitor);
    auto& regions = visitor.GetRegions();

    return_regions(regions);
}

void w_RTree_query_leaves()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    auto box = ((wrapper::Proxy<gs::Box>*)ves_toforeign(1))->obj;

    const auto& c = box->GetCube();
    const double min[] = { c.xmin, c.ymin, c.zmin, 0 };
    const double max[] = { c.xmax, c.ymax, c.zmax, 0 };
    spatialdb::Region region(min, max);

    tt::RegionVisitor visitor;
    rtree->ContainsWhatQuery(region, visitor);
    auto& regions = visitor.GetRegions();

    return_regions(regions);
}

void w_RTree_pick_aabbs()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    auto pos = wrapper::map_to_vec3(1);
    auto dir = wrapper::map_to_vec3(2);

    tt::PickVisitor visitor(pos, dir);
    rtree->LevelTraversal(visitor);

    auto& regions = visitor.GetRegions();
    return_regions(regions);
}

void w_RTree_pick_polys()
{
    auto rtree = ((wrapper::Proxy<spatialdb::RTree>*)ves_toforeign(0))->obj;
    auto pos = wrapper::map_to_vec3(1);
    auto dir = wrapper::map_to_vec3(2);

    tt::PickVisitor visitor(pos, dir);
    rtree->LevelTraversal(visitor);

    auto& polys = visitor.GetPolys();
    wrapper::return_foreign_list(polys, "geometry", "Polytope");
}

void w_RKey_allocate()
{
    auto key = std::make_shared<tt::BRepKey>();

    auto num = ves_argnum();
    if (num == 2)
    {
        sm::cube cube = ((wrapper::Proxy<gs::Box>*)ves_toforeign(1))->obj->GetCube();
        const double min[4] = { cube.xmin, cube.ymin, cube.zmin, 0 };
        const double max[4] = { cube.xmax, cube.ymax, cube.zmax, 0 };
        key->r.Combine(spatialdb::Point(min));
        key->r.Combine(spatialdb::Point(max));
    }

    auto proxy = (wrapper::Proxy<tt::BRepKey>*)ves_set_newforeign(0, 0, sizeof(wrapper::Proxy<tt::BRepKey>));
    proxy->obj = key;

}

int w_RKey_finalize(void* data)
{
    auto proxy = (wrapper::Proxy<tt::BRepKey>*)(data);
    proxy->~Proxy();
    return sizeof(wrapper::Proxy<tt::BRepKey>);
}

void w_RKey_region()
{
    auto rkey = ((wrapper::Proxy<tt::BRepKey>*)ves_toforeign(0))->obj;

    auto min = rkey->r.GetLow();
    auto max = rkey->r.GetHigh();

    sm::cube aabb;
    for (int i = 0; i < 3; ++i)
    {
        aabb.min[i] = static_cast<float>(min[i]);
        aabb.max[i] = static_cast<float>(max[i]);
    }

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("geometry", "Box");
    auto proxy = (wrapper::Proxy<gs::Box>*)ves_set_newforeign(0, 1, sizeof(wrapper::Proxy<gs::Box>));
    proxy->obj = std::make_shared<gs::Box>(aabb);
    ves_pop(1);
}

void w_RKey_id()
{
    auto rkey = ((wrapper::Proxy<tt::BRepKey>*)ves_toforeign(0))->obj;
    ves_set_number(0, static_cast<double>(rkey->id));
}

void w_RFile_allocate()
{
    const char* filename = ves_tostring(1);
    auto proxy = (wrapper::Proxy<spatialdb::DiskStorageManager>*)ves_set_newforeign(0, 0, sizeof(wrapper::Proxy<spatialdb::DiskStorageManager>));
    proxy->obj = std::make_shared<spatialdb::DiskStorageManager>(filename, false);
}

int w_RFile_finalize(void* data)
{
    auto proxy = (wrapper::Proxy<spatialdb::DiskStorageManager>*)(data);
    proxy->~Proxy();
    return sizeof(wrapper::Proxy<spatialdb::DiskStorageManager>);
}

}

namespace tt
{

VesselForeignMethodFn DbBindMethod(const char* signature)
{
    if (strcmp(signature, "RTree.load_from_file(_)") == 0) return w_RTree_load_from_file;
    if (strcmp(signature, "RTree.insert(_)") == 0) return w_RTree_insert;
    if (strcmp(signature, "RTree.query(_)") == 0) return w_RTree_query;
    if (strcmp(signature, "RTree.delete(_)") == 0) return w_RTree_delete;
    if (strcmp(signature, "RTree.clear()") == 0) return w_RTree_clear;
    if (strcmp(signature, "RTree.rollforward(_)") == 0) return w_RTree_rollforward;
    if (strcmp(signature, "RTree.rollback(_)") == 0) return w_RTree_rollback;
    if (strcmp(signature, "RTree.insert_with_time(_,_)") == 0) return w_RTree_insert_with_time;
    if (strcmp(signature, "RTree.query_with_time(_,_,_)") == 0) return w_RTree_query_with_time;
    if (strcmp(signature, "RTree.get_all_leaves()") == 0) return w_RTree_get_all_leaves;
    if (strcmp(signature, "RTree.query_leaves(_)") == 0) return w_RTree_query_leaves;
    if (strcmp(signature, "RTree.pick_aabbs(_,_)") == 0) return w_RTree_pick_aabbs;
    if (strcmp(signature, "RTree.pick_polys(_,_)") == 0) return w_RTree_pick_polys;

    if (strcmp(signature, "RKey.region()") == 0) return w_RKey_region;
    if (strcmp(signature, "RKey.id()") == 0) return w_RKey_id;

    return nullptr;
}

void DbBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "RTree") == 0)
    {
        methods->allocate = w_RTree_allocate;
        methods->finalize = w_RTree_finalize;
        return;
    }

    if (strcmp(class_name, "RKey") == 0)
    {
        methods->allocate = w_RKey_allocate;
        methods->finalize = w_RKey_finalize;
        return;
    }

    if (strcmp(class_name, "RFile") == 0)
    {
        methods->allocate = w_RFile_allocate;
        methods->finalize = w_RFile_finalize;
        return;
    }
}

}