#pragma once

#include <spatialdb/typedef.h>

#include <vector>
#include <memory>

namespace spatialdb { class ISpatialIndex; }
namespace ur { class VertexArray; }

namespace tt
{

struct SceneNode
{
	spatialdb::id_type id;

	std::shared_ptr<ur::VertexArray> vao = nullptr;

	std::weak_ptr<SceneNode> parent;
	std::vector<std::shared_ptr<SceneNode>> children;
};

class SceneTree
{
public:
	SceneTree() {}

	void Build(spatialdb::ISpatialIndex& si);

	auto GetRoot() const { return m_root; }

private:
	std::shared_ptr<SceneNode> m_root;

}; // SceneTree

}