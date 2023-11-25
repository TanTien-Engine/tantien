#pragma once

#include <brepdb/typedef.h>

#include <vector>
#include <memory>

namespace brepdb { class ISpatialIndex; }
namespace ur { class VertexArray; }

namespace tt
{

struct SceneNode
{
	brepdb::id_type id;

	std::shared_ptr<ur::VertexArray> vao = nullptr;

	std::weak_ptr<SceneNode> parent;
	std::vector<std::shared_ptr<SceneNode>> children;
};

class SceneTree
{
public:
	SceneTree() {}

	void Build(brepdb::ISpatialIndex& si);

	auto GetRoot() const { return m_root; }

private:
	std::shared_ptr<SceneNode> m_root;

}; // SceneTree

}