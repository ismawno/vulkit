#pragma once

#include "vkit/buffer/buffer.hpp"

namespace VKit
{
template <typename T> class HostVisibleBuffer
{
  public:
    struct Specs
    {
        VkDeviceSize Capacity;
        VkBufferUsageFlags Usage;
        VkDeviceSize MinimumAlignment = 1;
        VmaAllocationCreateFlags AllocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        bool Coherent = false;
    };

    struct SpecializedSpecs
    {
        VkDeviceSize Capacity;
        VmaAllocationCreateFlags AllocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        bool Coherent = false;
    };

    using VertexSpecs = SpecializedSpecs;
    using IndexSpecs = SpecializedSpecs;
    using UniformSpecs = SpecializedSpecs;
    using StorageSpecs = SpecializedSpecs;

    static Result<HostVisibleBuffer> Create(const Specs &p_Specs) noexcept
    {
        Buffer::Specs specs{};
        specs.InstanceCount = p_Specs.Capacity;
        specs.InstanceSize = sizeof(T);
        specs.Usage = p_Specs.Usage;
        specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
        specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        specs.AllocationInfo.flags = p_Specs.AllocationFlags;
        if (p_Specs.Coherent)
            specs.AllocationInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        const auto result = Buffer::Create(specs);
        if (!result)
            return Result<HostVisibleBuffer>::Error(result.GetError());

        return Result<HostVisibleBuffer>::Ok(result.GetValue());
    }

    static Result<HostVisibleBuffer> CreateVertexBuffer(const VertexSpecs &p_Specs) noexcept
    {
        Specs specs{};
        specs.Capacity = p_Specs.Capacity;
        specs.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        specs.Coherent = p_Specs.Coherent;
        return Create(specs);
    }

    static Result<HostVisibleBuffer> CreateIndexBuffer(const IndexSpecs &p_Specs,
                                                       const VkDeviceSize p_Alignment) noexcept
    {
        Specs specs{};
        specs.Capacity = p_Specs.Capacity;
        specs.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        specs.Coherent = p_Specs.Coherent;
        specs.MinimumAlignment = p_Alignment;
        return Create(specs);
    }

    static Result<HostVisibleBuffer> CreateUniformBuffer(const UniformSpecs &p_Specs,
                                                         const VkDeviceSize p_Alignment) noexcept
    {
        Specs specs{};
        specs.Capacity = p_Specs.Capacity;
        specs.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        specs.Coherent = p_Specs.Coherent;
        specs.MinimumAlignment = p_Alignment;
        return Create(specs);
    }

    explicit HostVisibleBuffer(const Buffer &p_Buffer) noexcept : m_Buffer(p_Buffer)
    {
    }

    void Destroy() noexcept
    {
        m_Buffer.Destroy();
    }
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
    {
        m_Buffer.SubmitForDeletion(p_Queue);
    }

    void Map() noexcept
    {
        m_Buffer.Map();
    }
    void Unmap() noexcept
    {
        m_Buffer.Unmap();
    }

    void WriteAt(const usize p_Index, const T *p_Data) noexcept
    {
        m_Buffer.WriteAt(p_Index, p_Data);
    }

    void Flush() noexcept
    {
        m_Buffer.Flush();
    }
    void FlushAt(const usize p_Index) noexcept
    {
        m_Buffer.FlushAt(p_Index);
    }

    void Invalidate() noexcept
    {
        m_Buffer.Invalidate();
    }
    void InvalidateAt(const usize p_Index) noexcept
    {
        m_Buffer.InvalidateAt(p_Index);
    }

    VkDescriptorBufferInfo GetDescriptorInfo() const noexcept
    {
        return m_Buffer.GetDescriptorInfo();
    }
    VkDescriptorBufferInfo GetDescriptorInfoAt(const usize p_Index) const noexcept
    {
        return m_Buffer.GetDescriptorInfoAt(p_Index);
    }

    T *GetData() const noexcept
    {
        return static_cast<T *>(m_Buffer.GetData());
    }
    T *ReadAt(const usize p_Index) const noexcept
    {
        return static_cast<T *>(m_Buffer.ReadAt(p_Index));
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