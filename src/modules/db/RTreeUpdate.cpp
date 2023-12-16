#include "RTreeUpdate.h"
#include "DB.h"
#include "BRepKey.h"
#include "BrepSerialize.h"
#include "modules/regen/PolyDiff.h"

#include <brepdb/RTree.h>
#include <brepdb/Point.h>
#include <polymesh3/Polytope.h>

namespace tt
{

brepdb::id_type NEXT_ID = 0;

std::shared_ptr<tt::BRepKey> RTreeUpdate::
Insert(const std::shared_ptr<brepdb::RTree>& rtree, const std::shared_ptr<pm3::Polytope>& poly)
{
    brepdb::id_type id;

    auto& map = tt::DB::Instance()->GetPoly2KeyMap();
    auto itr = map.find(poly);
    if (itr != map.end())
    {
        auto& key = itr->second;
        rtree->DeleteData(key->r, key->id);

        id = key->id;

        map.erase(itr);
    }
    else
    {
        id = NEXT_ID++;
    }

    uint8_t* data = nullptr;
    uint32_t length = 0;
    tt::BrepSerialize::BRepToByteArray(*poly, &data, length);

    brepdb::Region aabb;
    auto& pts = poly->Points();
    for (auto& p : pts)
    {
        auto src = p->pos.xyz;
        const double dst[4] = { src[0], src[1], src[2], 0 };
        aabb.Combine(brepdb::Point(dst));
    }

    rtree->InsertData(length, data, aabb, id);
    delete[] data;

    auto rkey = std::make_shared<tt::BRepKey>();
    rkey->r = aabb;
    rkey->id = id;

    map.insert({ poly, rkey });

    return rkey;
}

void RTreeUpdate::Clear(std::shared_ptr<brepdb::RTree>& rtree)
{
    auto& map = tt::DB::Instance()->GetPoly2KeyMap();
    for (auto& itr : map)
    {
        auto& key = itr.second;
        rtree->DeleteData(key->r, key->id);
    }
    map.clear();
}

void RTreeUpdate::RollForward(std::shared_ptr<brepdb::RTree>& rtree,
                              const std::shared_ptr<tt::PolyDiff>& diff)
{
    auto& map = tt::DB::Instance()->GetPoly2KeyMap();

    // add_list
    auto& add_list = diff->GetAddList();
    for (auto add_pair : add_list)
    {
        auto key = Insert(rtree, add_pair.second);
        map.insert({ add_pair.second, key });
    }

    // del_list
    auto& del_list = diff->GetDelList();
    for (auto del : del_list)
    {
        auto itr = map.find(del);
        if (itr != map.end())
        {
            auto& key = itr->second;
            rtree->DeleteData(key->r, key->id);

            map.erase(itr);
        }
    }

    // mod_list
    auto& mod_list = diff->GetModList();
    for (auto mod_pair : mod_list)
    {
        auto itr = map.find(mod_pair.first);
        if (itr != map.end())
        {
            auto& key = itr->second;
            rtree->DeleteData(key->r, key->id);

            map.erase(itr);
        }

        auto key = Insert(rtree, mod_pair.second);
        map.insert({ mod_pair.second, key });
    }
}

void RTreeUpdate::RollBack(std::shared_ptr<brepdb::RTree>& rtree, 
                           const std::shared_ptr<tt::PolyDiff>& diff)
{
    auto& map = tt::DB::Instance()->GetPoly2KeyMap();

    // add_list
    auto& add_list = diff->GetAddList();
    for (auto add : add_list)
    {
        auto itr = map.find(add.second);
        if (itr != map.end())
        {
            auto& key = itr->second;
            rtree->DeleteData(key->r, key->id);

            map.erase(itr);
        }
    }

    // del_list
    auto& del_list = diff->GetDelList();
    for (auto del : del_list)
    {
        auto key = Insert(rtree, del);
        map.insert({ del, key });
    }

    // mod_list
    auto& mod_list = diff->GetModList();
    for (auto mod_pair : mod_list)
    {
        auto itr = map.find(mod_pair.second);
        if (itr != map.end())
        {
            auto& key = itr->second;
            rtree->DeleteData(key->r, key->id);

            map.erase(itr);
        }

        auto key = Insert(rtree, mod_pair.first);
        map.insert({ mod_pair.first, key });
    }
}

}