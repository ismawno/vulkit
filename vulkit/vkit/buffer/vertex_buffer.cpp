#include "vkit/core/pch.hpp"
#include "vkit/buffer/vertex_buffer.hpp"

namespace VKit
{
template <Dimension D>
VertexBuffer<D>::VertexBuffer(const std::span<const Vertex<D>> p_Vertices) noexcept
    : DeviceBuffer<Vertex<D>>(p_Vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
{
}

template <Dimension D>
void VertexBuffer<D>::Bind(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset) const noexcept
{
    const VkBuffer buffer = this->GetBuffer();
    vkCmdBindVertexBuffers(p_CommandBuffer, 0, 1, &buffer, &p_Offset);
}

template class VertexBuffer<D2>;
template class VertexBuffer<D3>;

template <Dimension D> static Buffer::Specs createBufferSpecs(const usize p_Size)
{
    Buffer::Specs specs{};
    specs.InstanceCount = p_Size;
    specs.InstanceSize = sizeof(Vertex<D>);
    specs.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    specs.AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    return specs;
}

template <Dimension D>
MutableVertexBuffer<D>::MutableVertexBuffer(const std::span<const Vertex<D>> p_Vertices) noexcept
    : Buffer(createBufferSpecs<D>(p_Vertices.size()))
{
    Map();
    Write(p_Vertices);
    Flush();
}
template <Dimension D>
MutableVertexBuffer<D>::MutableVertexBuffer(const usize p_Size) noexcept : Buffer(createBufferSpecs<D>(p_Size))
{
    Map();
}

template <Dimension D>
void MutableVertexBuffer<D>::Bind(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset) const noexcept
{
    const VkBuffer buffer = this->GetBuffer();
    vkCmdBindVertexBuffers(p_CommandBuffer, 0, 1, &buffer, &p_Offset);
}

template <Dimension D> void MutableVertexBuffer<D>::Write(const std::span<const Vertex<D>> p_Vertices)
{
    Buffer::Write(p_Vertices.data(), p_Vertices.size() * sizeof(Vertex<D>));
}

template class MutableVertexBuffer<D2>;
template class MutableVertexBuffer<D3>;

} // namespace VKit