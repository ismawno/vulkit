#pragma once

#include "vkit/buffer/buffer.hpp"
#include "vkit/backend/command_pool.hpp"

namespace VKit
{
template <typename T> class DeviceLocalBuffer
{
  public:
    struct Specs
    {
        std::span<const T> Data;
        VkBufferUsageFlags Usage;
        CommandPool *CommandPool;
        VkQueue Queue;
        VkDeviceSize MinimumAlignment = 1;
        VmaAllocationCreateFlags AllocationFlags = 0;
    };

    struct SpecializedSpecs
    {
        std::span<const T> Data;
        CommandPool *CommandPool;
        VkQueue Queue;
        VmaAllocationCreateFlags AllocationFlags = 0;
    };

    using VertexSpecs = SpecializedSpecs;
    using IndexSpecs = SpecializedSpecs;
    using UniformSpecs = SpecializedSpecs;
    using StorageSpecs = SpecializedSpecs;

    static Result<DeviceLocalBuffer> Create(const Specs &p_Specs) noexcept
    {
        Buffer::Specs specs{};
        specs.InstanceCount = p_Specs.Data.size();
        specs.InstanceSize = sizeof(T);
        specs.Usage = p_Specs.Usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        specs.AllocationInfo.flags = p_Specs.AllocationFlags;

        const auto result1 = Buffer::Create(specs);
        if (!result1)
            return Result<DeviceLocalBuffer>::Error(result1.GetError().Result, "Failed to create main device buffer");

        const Buffer &buffer = result1.GetValue();

        Buffer::Specs stagingSpecs = specs;
        stagingSpecs.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingSpecs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

        // WARNING: Is this a good option even when MinimumAlignment is not 1?
        stagingSpecs.AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

        auto result2 = Buffer::Create(stagingSpecs);
        if (!result2)
            return Result<DeviceLocalBuffer>::Error(result2.GetError().Result, "Failed to create staging buffer");

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
            return Result<DeviceLocalBuffer>::Error(result3.GetError().Result, "Failed to copy data to main buffer");

        return Result<DeviceLocalBuffer>::Ok(buffer);
    }

    static Result<DeviceLocalBuffer> CreateVertexBuffer(const VertexSpecs &p_Specs) noexcept
    {
        Specs specs{};
        specs.Data = p_Specs.Data;
        specs.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        specs.CommandPool = p_Specs.CommandPool;
        specs.Queue = p_Specs.Queue;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        return Create(specs);
    }

    static Result<DeviceLocalBuffer> CreateIndexBuffer(const IndexSpecs &p_Specs) noexcept
    {
        Specs specs{};
        specs.Data = p_Specs.Data;
        specs.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        specs.CommandPool = p_Specs.CommandPool;
        specs.Queue = p_Specs.Queue;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        return Create(specs);
    }

    static Result<DeviceLocalBuffer> CreateUniformBuffer(const UniformSpecs &p_Specs) noexcept
    {
        Specs specs{};
        specs.Data = p_Specs.Data;
        specs.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        specs.CommandPool = p_Specs.CommandPool;
        specs.Queue = p_Specs.Queue;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        return Create(specs);
    }

    static Result<DeviceLocalBuffer> CreateStorageBuffer(const StorageSpecs &p_Specs) noexcept
    {
        Specs specs{};
        specs.Data = p_Specs.Data;
        specs.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        specs.CommandPool = p_Specs.CommandPool;
        specs.Queue = p_Specs.Queue;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        return Create(specs);
    }

    explicit DeviceLocalBuffer(const Buffer &p_Buffer) noexcept : m_Buffer(p_Buffer)
    {
    }

    void Destroy() noexcept
    {
        m_Buffer.Destroy();
    }
    void SubmitForDeletion(DeletionQueue &p_Queue) noexcept
    {
        m_Buffer.SubmitForDeletion(p_Queue);
    }

    void BindAsIndexBuffer(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset = 0) const noexcept
    {
        if constexpr (std::is_same_v<T, u8>)
            vkCmdBindIndexBuffer(p_CommandBuffer, m_Buffer.GetBuffer(), p_Offset, VK_INDEX_TYPE_UINT8_EXT);
        else if constexpr (std::is_same_v<T, u16>)
            vkCmdBindIndexBuffer(p_CommandBuffer, m_Buffer.GetBuffer(), p_Offset, VK_INDEX_TYPE_UINT16);
        else if constexpr (std::is_same_v<T, u32>)
            vkCmdBindIndexBuffer(p_CommandBuffer, m_Buffer.GetBuffer(), p_Offset, VK_INDEX_TYPE_UINT32);
#ifdef TKIT_ENABLE_ASSERTS
        else
        {
            TKIT_ERROR("Invalid index type");
        }
#endif
    }

    void BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const u32 p_BindingCount = 1,
                            const u32 p_FirstBinding = 0, const VkDeviceSize p_Offset = 0) const noexcept
    {
        const VkBuffer buffer = m_Buffer.GetBuffer();
        vkCmdBindVertexBuffers(p_CommandBuffer, p_FirstBinding, p_BindingCount, &buffer, &p_Offset);
    }

    VkDescriptorBufferInfo GetDescriptorInfo() const noexcept
    {
        return m_Buffer.GetDescriptorInfo();
    }
    VkDescriptorBufferInfo GetDescriptorInfoAt(const usize p_Index) const noexcept
    {
        return m_Buffer.GetDescriptorInfoAt(p_Index);
    }

    VkBuffer GetBuffer() const noexcept
    {
        return m_Buffer.GetBuffer();
    }
    explicit(false) operator VkBuffer() const noexcept
    {
        return m_Buffer.GetBuffer();
    }
    explicit(false) operator bool() const noexcept
    {
        return static_cast<bool>(m_Buffer);
    }

    const Buffer::Info &GetInfo() const noexcept
    {
        return m_Buffer.GetInfo();
    }

  private:
    Buffer m_Buffer;
};
} // namespace VKit