#pragma once

#include "vkit/backend/system.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/core/vma.hpp"

#include "tkit/memory/block_allocator.hpp"

#include <vulkan/vulkan.hpp>

// User may not use mutable buffer methods if the buffer is not/cannot be mapped

namespace VKit
{
class CommandPool;
class VKIT_API Buffer
{
  public:
    struct Specs
    {
        VmaAllocator Allocator = VK_NULL_HANDLE;
        VkDeviceSize InstanceCount;
        VkDeviceSize InstanceSize;
        VkBufferUsageFlags Usage;
        VmaAllocationCreateInfo AllocationInfo;
        VkDeviceSize MinimumAlignment = 1;
    };

    struct Info
    {
        VmaAllocator Allocator;
        VmaAllocation Allocation;

        VkDeviceSize InstanceSize;
        VkDeviceSize InstanceCount;
        VkDeviceSize AlignedInstanceSize;
        VkDeviceSize Size;
    };

    static Result<Buffer> Create(const Specs &p_Specs) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) noexcept;

    void Map() noexcept;
    void Unmap() noexcept;

    void Write(const void *p_Data, VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0) noexcept;
    void WriteAt(usize p_Index, const void *p_Data) noexcept;

    void Flush(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0) noexcept;
    void FlushAt(usize p_Index) noexcept;

    void Invalidate(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0) noexcept;
    void InvalidateAt(usize p_Index) noexcept;

    VkDescriptorBufferInfo GetDescriptorInfo(VkDeviceSize p_Size = VK_WHOLE_SIZE,
                                             VkDeviceSize p_Offset = 0) const noexcept;
    VkDescriptorBufferInfo GetDescriptorInfoAt(usize p_Index) const noexcept;

    void *GetData() const noexcept;
    void *ReadAt(usize p_Index) const noexcept;

    VulkanResult CopyFrom(const Buffer &p_Source, CommandPool &p_Pool, VkQueue p_Queue) noexcept;

    VkBuffer GetBuffer() const noexcept;
    explicit(false) operator VkBuffer() const noexcept;
    explicit(false) operator bool() const noexcept;

    const Info &GetInfo() const noexcept;

  private:
    explicit Buffer(VkBuffer p_Buffer, const Info &p_Info) noexcept;

    void *m_Data = nullptr;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    Info m_Info;
};

template <typename T> struct DeviceLocalBufferSpecs
{
    std::span<const T> Data;
    VkBufferUsageFlags Usage;
    CommandPool *CommandPool;
    VkQueue Queue;
    VkDeviceSize MinimumAlignment = 1;
};

template <typename T> struct TightBufferSpecs
{
    std::span<const T> Data;
    CommandPool *CommandPool;
    VkQueue Queue;
};
template <typename T> struct AlignedBufferSpecs
{
    std::span<const T> Data;
    CommandPool *CommandPool;
    VkQueue Queue;
    VkDeviceSize MinimumAlignment;
};

template <std::integral Index> using IndexBufferSpecs = TightBufferSpecs<Index>;
template <typename Vertex> using VertexBufferSpecs = TightBufferSpecs<Vertex>;
template <typename T> using UniformBufferSpecs = AlignedBufferSpecs<T>;
template <typename T> using StorageBufferSpecs = AlignedBufferSpecs<T>;

template <typename T> Result<Buffer> CreateDeviceLocalBuffer(const DeviceLocalBufferSpecs<T> &p_Specs) noexcept
{
    Buffer::Specs specs{};
    specs.InstanceCount = p_Specs.Data.size();
    specs.InstanceSize = sizeof(T);
    specs.Usage = p_Specs.Usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const auto result1 = Buffer::Create(specs);
    if (!result1)
        return Result<Buffer>::Error(result1.GetError().Result, "Failed to create main device buffer");

    const Buffer &buffer = result1.GetValue();

    Buffer::Specs stagingSpecs = specs;
    stagingSpecs.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingSpecs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // WARNING: Is this a good option even when MinimumAlignment is not 1?
    stagingSpecs.AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    auto result2 = Buffer::Create(stagingSpecs);
    if (!result2)
        return Result<Buffer>::Error(result2.GetError().Result, "Failed to create staging buffer");

    Buffer &stagingBuffer = result2.GetValue();
    stagingBuffer.Map();
    if (p_Specs.MinimumAlignment == 1)
        stagingBuffer.Write(p_Specs.Data.data());
    else
        for (usize i = 0; i < p_Specs.Data.size(); ++i)
            stagingBuffer.WriteAt(i, &p_Specs.Data[i]);
    stagingBuffer.Flush();
    stagingBuffer.Unmap();

    const auto result3 = buffer.CopyFrom(stagingBuffer, *p_Specs.CommandPool, p_Specs.Queue);
    stagingBuffer.Destroy();
    if (!result3)
        return Result<Buffer>::Error(result3.GetError().Result, "Failed to copy data to main buffer");

    return Result<Buffer>::Ok(buffer);
}

template <std::integral Index> Result<Buffer> CreateIndexBuffer(const IndexBufferSpecs<Index> &p_Specs) noexcept
{
    DeviceLocalBufferSpecs<Index> specs{};
    specs.Data = p_Specs.Data;
    specs.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    specs.CommandPool = p_Specs.CommandPool;
    specs.Queue = p_Specs.Queue;
    return CreateDeviceLocalBuffer(specs);
}

template <std::integral Index> Result<Buffer> CreateMutableIndexBuffer(const usize p_Capacity) noexcept
{
    Buffer::Specs specs{};
    specs.InstanceCount = p_Capacity;
    specs.InstanceSize = sizeof(Index);
    specs.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    specs.AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    return Buffer::Create(specs);
}

template <typename Vertex> Result<Buffer> CreateVertexBuffer(const VertexBufferSpecs<Vertex> &p_Specs) noexcept
{
    DeviceLocalBufferSpecs<Vertex> specs{};
    specs.Data = p_Specs.Data;
    specs.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    specs.CommandPool = p_Specs.CommandPool;
    specs.Queue = p_Specs.Queue;
    return CreateDeviceLocalBuffer(specs);
}

template <typename Vertex> Result<Buffer> CreateMutableVertexBuffer(const usize p_Capacity) noexcept
{
    Buffer::Specs specs{};
    specs.InstanceCount = p_Capacity;
    specs.InstanceSize = sizeof(Vertex);
    specs.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    specs.AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    return Buffer::Create(specs);
}

template <typename T> Result<Buffer> CreateUniformBuffer(const UniformBufferSpecs<T> &p_Specs) noexcept
{
    DeviceLocalBufferSpecs<T> specs{};
    specs.Data = p_Specs.Data;
    specs.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    specs.CommandPool = p_Specs.CommandPool;
    specs.Queue = p_Specs.Queue;
    specs.MinimumAlignment = p_Specs.MinimumAlignment;
    return CreateDeviceLocalBuffer(specs);
}

template <typename T>
Result<Buffer> CreateMutableUniformBuffer(const usize p_Capacity, const VkDeviceSize p_Alignment) noexcept
{
    Buffer::Specs specs{};
    specs.InstanceCount = p_Capacity;
    specs.InstanceSize = sizeof(T);
    specs.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    specs.AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    specs.MinimumAlignment = p_Alignment;
    return Buffer::Create(specs);
}

template <typename T> Result<Buffer> CreateStorageBuffer(const StorageBufferSpecs<T> &p_Specs) noexcept
{
    DeviceLocalBufferSpecs<T> specs{};
    specs.Data = p_Specs.Data;
    specs.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    specs.CommandPool = p_Specs.CommandPool;
    specs.Queue = p_Specs.Queue;
    specs.MinimumAlignment = p_Specs.MinimumAlignment;
    return CreateDeviceLocalBuffer(specs);
}

template <typename T>
Result<Buffer> CreateMutableStorageBuffer(const usize p_Capacity, const VkDeviceSize p_Alignment) noexcept
{
    Buffer::Specs specs{};
    specs.InstanceCount = p_Capacity;
    specs.InstanceSize = sizeof(T);
    specs.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    specs.AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    specs.MinimumAlignment = p_Alignment;
    return Buffer::Create(specs);
}

template <std::integral Index>
void BindIndexBuffer(const Buffer &p_Buffer, VkCommandBuffer p_CommandBuffer, VkDeviceSize p_Offset = 0) noexcept
{
    if constexpr (std::is_same_v<Index, u8>)
        vkCmdBindIndexBuffer(p_CommandBuffer, p_Buffer.GetBuffer(), p_Offset, VK_INDEX_TYPE_UINT8_EXT);
    else if constexpr (std::is_same_v<Index, u16>)
        vkCmdBindIndexBuffer(p_CommandBuffer, p_Buffer.GetBuffer(), p_Offset, VK_INDEX_TYPE_UINT16);
    else if constexpr (std::is_same_v<Index, u32>)
        vkCmdBindIndexBuffer(p_CommandBuffer, p_Buffer.GetBuffer(), p_Offset, VK_INDEX_TYPE_UINT32);
    else
        static_assert(false, "Invalid index type");
}
void BindVertexBuffer(const Buffer &p_Buffer, VkCommandBuffer p_CommandBuffer, u32 p_BindingCount = 1,
                      u32 p_FirstBinding = 0, VkDeviceSize p_Offset = 0) noexcept;

} // namespace VKit