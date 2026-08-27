#pragma once

#include "voxel_game/voxel/VisibleFaceMesher.h"

#include <cstddef>
#include <memory>
#include <string>

namespace ur
{
class Device;
class VertexArray;
class VertexBuffer;
class IndexBuffer;
}

namespace tt::voxel_game
{

// Owns UniRender OpenGL buffers and VAO. reset() must be called while the
// GLFW context is current so the backing GL objects are released safely.
class VoxelGpuMesh
{
public:
    bool upload(const ur::Device& device, const OpaqueMesh& mesh, std::string& error);
    void draw() const;
    void reset();

    std::size_t index_count() const;

private:
    std::shared_ptr<ur::VertexArray> m_vertex_array;
    std::shared_ptr<ur::VertexBuffer> m_vertex_buffer;
    std::shared_ptr<ur::IndexBuffer> m_index_buffer;
    std::size_t m_index_count = 0;
};

} // namespace tt::voxel_game
