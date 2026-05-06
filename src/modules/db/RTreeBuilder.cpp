#include "RTreeBuilder.h"
#include "BrepSerialize.h"

#include <SM_Vector.h>
#include <spatialdb/RTree.h>
#include <spatialdb/Region.h>
#include <spatialdb/Point.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <filesystem>
#include <iostream>
#include <map>

namespace
{

struct Vertex
{
	float pos[3];
	float normal[3];
	float texcoord[2];
};

}

namespace tt
{

void RTreeBuilder::FromModeling(spatialdb::RTree& rtree, const char* filepath)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn, err;
    auto dir = std::filesystem::path(filepath).parent_path().string() + "/";
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath, dir.c_str(), false);

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        exit(1);
    }

	for (auto& shape : shapes)
	{
		auto& mesh = shape.mesh;

		std::vector<sm::vec3> points;
		std::vector<std::vector<size_t>> faces;

		std::map<size_t, size_t> map_vert_idx;
		size_t index_offset = 0;
		for (size_t f = 0, n = mesh.num_face_vertices.size(); f < n; f++)
		{
			int fv = mesh.num_face_vertices[f];

			std::vector<size_t> face;
			face.reserve(fv);

			for (size_t v = 0; v < fv; v++) 
			{
				tinyobj::index_t idx = mesh.indices[index_offset + v];

				size_t p_idx;

				auto itr = map_vert_idx.find(idx.vertex_index);
				if (itr == map_vert_idx.end())
				{
					p_idx = points.size();

					map_vert_idx.insert({ idx.vertex_index, points.size() });

					const tinyobj::vec3_t& v = attrib.vertices[idx.vertex_index];
					points.emplace_back(sm::vec3(v.x, v.y, v.z) * 0.01f);
				}
				else
				{
					p_idx = itr->second;
				}

				face.push_back(p_idx);
			}
			index_offset += fv;

			faces.push_back(face);
		}

		uint8_t* data = nullptr;
		uint32_t length = 0;
		BrepSerialize::BRepToByteArray(points, faces, &data, length);

		spatialdb::id_type id = 0;

		spatialdb::Region aabb;
		for (auto& p : points)
		{
			auto src = p.xyz;
			const double dst[4] = { src[0], src[1], src[2], 0 };
			aabb.Combine(spatialdb::Point(dst));
		}

		rtree.InsertData(length, data, aabb, id);
		delete[] data;
	}
}

}