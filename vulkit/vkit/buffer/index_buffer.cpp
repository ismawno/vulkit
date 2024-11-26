#include "vkit/core/pch.hpp"
#include "vkit/buffer/index_buffer.hpp"

namespace VKit
{
IndexBuffer::IndexBuffer(const std::span<const Index> p_Indices) noexcept
    : DeviceBuffer<Index>(p_Indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
{
}

void IndexBuffer::Bind(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset) const noexcept
{
    vkCmdBindIndexBuffer(p_CommandBuffer, GetBuffer(), p_Offset, VK_INDEX_TYPE_UINT32);
}

static Buffer::Specs createBufferSpecs(const usize p_Size)
{
    Buffer::Specs specs{};
    specs.InstanceCount = p_Size;
    specs.InstanceSize = sizeof(Index);
    specs.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    specs.AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    return specs;
}

MutableIndexBuffer::MutableIndexBuffer(const std::span<const Index> p_Indices) noexcept
    : Buffer(createBufferSpecs(p_Indices.size()))
{
    Map();
    Write(p_Indices);
    Flush();
}

MutableIndexBuffer::MutableIndexBuffer(const usize p_Size) noexcept : Buffer(createBufferSpecs(p_Size))
{
    Map();
}

void MutableIndexBuffer::Bind(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset) const noexcept
{
    vkCmdBindIndexBuffer(p_CommandBuffer, GetBuffer(), p_Offset, VK_INDEX_TYPE_UINT32);
}

void MutableIndexBuffer::Write(const std::span<const Index> p_Indices)
{
    Buffer::Write(p_Indices.data(), p_Indices.size() * sizeof(Index));
}

} // namespace VKit