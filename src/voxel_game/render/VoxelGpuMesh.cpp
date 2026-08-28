#include "voxel_game/render/VoxelGpuMesh.h"

#include <GL/gl3w.h>

#include <unirender/BufferUsageHint.h>
#include <unirender/ComponentDataType.h>
#include <unirender/Device.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexArray.h>
#include <unirender/VertexBuffer.h>
#include <unirender/VertexInputAttribute.h>
#include <unirender/opengl/VertexArray.h>

#include <limits>
#include <memory>
#include <cstddef>

namespace tt::voxel_game
{
namespace
{

bool byte_count(std::size_t element_count, std::size_t element_size, int& result)
{
    if (element_count == 0 || element_count >
        static_cast<std::size_t>((std::numeric_limits<int>::max)()) / element_size)
    {
        return false;
    }
    result = static_cast<int>(element_count * element_size);
    return true;
}

} // namespace

bool VoxelGpuMesh::upload(const ur::Device& device, const OpaqueMesh& mesh, std::string& error)
{
    reset();
    if (mesh.vertices.empty() && mesh.indices.empty())
    {
        return true;
    }
    int vertex_bytes = 0;
    int index_bytes = 0;
    if (!byte_count(mesh.vertices.size(), sizeof(VoxelVertex), vertex_bytes) ||
        !byte_count(mesh.indices.size(), sizeof(std::uint32_t), index_bytes))
    {
        error = "voxel mesh is empty or exceeds UniRender buffer size";
        return false;
    }

    auto vertex_buffer = device.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vertex_bytes);
    auto index_buffer = device.CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, index_bytes);
    auto vertex_array = device.CreateVertexArray();
    if (!vertex_buffer || !index_buffer || !vertex_array)
    {
        error = "UniRender could not create voxel GPU resources";
        return false;
    }

    vertex_buffer->ReadFromMemory(mesh.vertices.data(), vertex_bytes, 0);
    index_buffer->ReadFromMemory(mesh.indices.data(), index_bytes, 0);
    index_buffer->SetDataType(ur::IndexBufferDataType::UnsignedInt);
    index_buffer->SetCount(static_cast<int>(mesh.indices.size()));

    vertex_array->SetVertexBuffer(vertex_buffer);
    vertex_array->SetIndexBuffer(index_buffer);
    vertex_array->SetVertexBufferAttrs({
        std::make_shared<ur::VertexInputAttribute>(
            0, ur::ComponentDataType::Float, 3,
            static_cast<int>(offsetof(VoxelVertex, position)), static_cast<int>(sizeof(VoxelVertex))),
        std::make_shared<ur::VertexInputAttribute>(
            1, ur::ComponentDataType::Float, 3,
            static_cast<int>(offsetof(VoxelVertex, normal)), static_cast<int>(sizeof(VoxelVertex))),
        std::make_shared<ur::VertexInputAttribute>(
            2, ur::ComponentDataType::Float, 4,
            static_cast<int>(offsetof(VoxelVertex, color)), static_cast<int>(sizeof(VoxelVertex))),
    });

    const auto gl_vertex_array = std::static_pointer_cast<ur::opengl::VertexArray>(vertex_array);
    gl_vertex_array->Bind();
    gl_vertex_array->Clean();
    if (glGetError() != GL_NO_ERROR)
    {
        error = "OpenGL failed while configuring the UniRender voxel VAO";
        return false;
    }

    m_vertex_array = std::move(vertex_array);
    m_vertex_buffer = std::move(vertex_buffer);
    m_index_buffer = std::move(index_buffer);
    m_index_count = mesh.indices.size();
    return true;
}

void VoxelGpuMesh::draw() const
{
    if (!m_vertex_array || m_index_count == 0)
    {
        return;
    }

    const auto gl_vertex_array = std::static_pointer_cast<ur::opengl::VertexArray>(m_vertex_array);
    gl_vertex_array->Bind();
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(m_index_count),
                   GL_UNSIGNED_INT,
                   nullptr);
}

void VoxelGpuMesh::reset()
{
    m_vertex_array.reset();
    m_index_buffer.reset();
    m_vertex_buffer.reset();
    m_index_count = 0;
}

std::size_t VoxelGpuMesh::index_count() const
{
    return m_index_count;
}

} // namespace tt::voxel_game
