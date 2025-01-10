#pragma once

#include "vkit/buffer/buffer.hpp"

namespace VKit
{
/**
 * @brief Represents a buffer stored in host-visible memory.
 *
 * Provides methods for writing data, flushing, invalidating memory, and binding as
 * vertex or index buffers. Designed for use cases requiring frequent CPU-GPU data transfer.
 *
 * @tparam T The type of data stored in the buffer.
 */
template <typename T> class HostVisibleBuffer
{
  public:
    struct Specs
    {
        VmaAllocator Allocator = VK_NULL_HANDLE;
        VkDeviceSize Capacity;
        VkBufferUsageFlags Usage;
        VkDeviceSize MinimumAlignment = 1;
        VmaAllocationCreateFlags AllocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        bool Coherent = false;
    };

    struct SpecializedSpecs
    {
        VmaAllocator Allocator = VK_NULL_HANDLE;
        VkDeviceSize Capacity;
        VmaAllocationCreateFlags AllocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        bool Coherent = false;
    };

    using VertexSpecs = SpecializedSpecs;
    using IndexSpecs = SpecializedSpecs;
    using UniformSpecs = SpecializedSpecs;
    using StorageSpecs = SpecializedSpecs;

    /**
     * @brief Creates a host-visible buffer with the specified settings.
     *
     * Allocates and maps a Vulkan buffer in host-visible memory based on the provided specifications.
     *
     * @param p_Specs The specifications for the buffer.
     * @return A result containing the created HostVisibleBuffer or an error.
     */
    static Result<HostVisibleBuffer> Create(const Specs &p_Specs) noexcept
    {
        Buffer::Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.InstanceCount = p_Specs.Capacity;
        specs.InstanceSize = sizeof(T);
        specs.Usage = p_Specs.Usage;
        specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
        specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        specs.AllocationInfo.flags = p_Specs.AllocationFlags;
        if (p_Specs.Coherent)
            specs.AllocationInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        specs.MinimumAlignment = p_Specs.MinimumAlignment;

        const auto result = Buffer::Create(specs);
        if (!result)
            return Result<HostVisibleBuffer>::Error(result.GetError());

        return Result<HostVisibleBuffer>::Ok(result.GetValue());
    }

    /**
     * @brief Creates a host-visible vertex buffer.
     *
     * Configures the buffer for vertex data and allocates memory in host-visible space.
     *
     * @param p_Specs The specifications for the vertex buffer.
     * @return A result containing the created vertex buffer or an error.
     */
    static Result<HostVisibleBuffer> CreateVertexBuffer(const VertexSpecs &p_Specs) noexcept
    {
        Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.Capacity = p_Specs.Capacity;
        specs.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        specs.Coherent = p_Specs.Coherent;
        return Create(specs);
    }

    /**
     * @brief Creates a host-visible index buffer.
     *
     * Configures the buffer for index data and allocates memory in host-visible space.
     *
     * @param p_Specs The specifications for the index buffer.
     * @return A result containing the created index buffer or an error.
     */
    static Result<HostVisibleBuffer> CreateIndexBuffer(const IndexSpecs &p_Specs) noexcept
    {
        Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.Capacity = p_Specs.Capacity;
        specs.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        specs.Coherent = p_Specs.Coherent;
        return Create(specs);
    }

    /**
     * @brief Creates a host-visible uniform buffer with specified alignment.
     *
     * Configures the buffer for uniform data and allocates memory with the required alignment.
     *
     * @param p_Specs The specifications for the uniform buffer.
     * @param p_Alignment The minimum alignment for the buffer.
     * @return A result containing the created uniform buffer or an error.
     */
    static Result<HostVisibleBuffer> CreateUniformBuffer(const UniformSpecs &p_Specs,
                                                         const VkDeviceSize p_Alignment) noexcept
    {
        Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.Capacity = p_Specs.Capacity;
        specs.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        specs.Coherent = p_Specs.Coherent;
        specs.MinimumAlignment = p_Alignment;
        return Create(specs);
    }

    /**
     * @brief Creates a host-visible storage buffer with specified alignment.
     *
     * Configures the buffer for storage data and allocates memory with the required alignment.
     *
     * @param p_Specs The specifications for the storage buffer.
     * @param p_Alignment The minimum alignment for the buffer.
     * @return A result containing the created storage buffer or an error.
     */
    static Result<HostVisibleBuffer> CreateStorageBuffer(const StorageSpecs &p_Specs,
                                                         const VkDeviceSize p_Alignment) noexcept
    {
        Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.Capacity = p_Specs.Capacity;
        specs.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        specs.Coherent = p_Specs.Coherent;
        specs.MinimumAlignment = p_Alignment;
        return Create(specs);
    }

    HostVisibleBuffer() noexcept = default;
    explicit HostVisibleBuffer(const Buffer &p_Buffer) noexcept : m_Buffer(p_Buffer)
    {
        m_Buffer.Map();
    }

    void Destroy() noexcept
    {
        m_Buffer.Destroy();
    }
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
    {
        m_Buffer.SubmitForDeletion(p_Queue);
    }

    /**
     * @brief Writes data to the buffer.
     *
     * Copies the given data into the buffer. The buffer must have a trivial alignment
     * requirement to use this method.
     *
     * @param p_Data A span containing the data to write.
     */
    void Write(const std::span<const T> p_Data) noexcept
    {
        TKIT_ASSERT(m_Buffer.GetInfo().AlignedInstanceSize == m_Buffer.GetInfo().InstanceSize,
                    "[VULKIT] Cannot 'mindlessly' write data to a buffer with a non-trivial alignment requirement");
        m_Buffer.Write(p_Data.data(), p_Data.size() * sizeof(T));
    }

    /**
     * @brief Writes data to the buffer at the specified index.
     *
     * Copies the provided data into the buffer at the specified index.
     * The buffer must be mapped before calling this method.
     *
     * Automatically handles alignment requirements.
     *
     * @param p_Index The index of the buffer instance to write to.
     * @param p_Data A pointer to the data to write.
     */
    void WriteAt(const u32 p_Index, const T *p_Data) noexcept
    {
        m_Buffer.WriteAt(p_Index, p_Data);
    }

    void Flush() noexcept
    {
        m_Buffer.Flush();
    }
    void FlushAt(const u32 p_Index) noexcept
    {
        m_Buffer.FlushAt(p_Index);
    }

    void Invalidate() noexcept
    {
        m_Buffer.Invalidate();
    }
    void InvalidateAt(const u32 p_Index) noexcept
    {
        m_Buffer.InvalidateAt(p_Index);
    }

    VkDescriptorBufferInfo GetDescriptorInfo() const noexcept
    {
        return m_Buffer.GetDescriptorInfo();
    }
    VkDescriptorBufferInfo GetDescriptorInfoAt(const u32 p_Index) const noexcept
    {
        return m_Buffer.GetDescriptorInfoAt(p_Index);
    }

    T *GetData() const noexcept
    {
        return static_cast<T *>(m_Buffer.GetData());
    }
    T *ReadAt(const u32 p_Index) const noexcept
    {
        return static_cast<T *>(m_Buffer.ReadAt(p_Index));
    }

    /**
     * @brief Binds the buffer as an index buffer to a command buffer.
     *
     * Automatically determines the index type (u8, u16, or u32) based on the buffer's template parameter.
     *
     * @param p_CommandBuffer The command buffer to bind the index buffer to.
     * @param p_Offset The offset within the buffer (default: 0).
     */
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

    /**
     * @brief Binds the buffer as a vertex buffer to a command buffer.
     *
     * @param p_CommandBuffer The command buffer to bind the vertex buffer to.
     * @param p_Offset The offset within the buffer (default: 0).
     */
    void BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset = 0) const noexcept
    {
        const VkBuffer buffer = m_Buffer.GetBuffer();
        vkCmdBindVertexBuffers(p_CommandBuffer, 0, 1, &buffer, &p_Offset);
    }

    /**
     * @brief Binds multiple buffers as vertex buffers to a command buffer.
     *
     * @param p_CommandBuffer The command buffer to bind the vertex buffers to.
     * @param p_Buffers A span containing the buffers to bind.
     * @param p_Offsets A span containing the offsets within the buffers (default: 0).
     */
    static void BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const std::span<const VkBuffer> p_Buffers,
                                   const std::span<const VkDeviceSize> p_Offsets = {}) noexcept
    {
        if (!p_Offsets.empty())
            vkCmdBindVertexBuffers(p_CommandBuffer, 0, static_cast<u32>(p_Buffers.size()), p_Buffers.data(),
                                   p_Offsets.data());
        else
            vkCmdBindVertexBuffers(p_CommandBuffer, 0, static_cast<u32>(p_Buffers.size()), p_Buffers.data(), nullptr);
    }

    /**
     * @brief Binds a buffer as a vertex buffer to a command buffer.
     *
     * @param p_CommandBuffer The command buffer to bind the vertex buffer to.
     * @param p_Offset The offset within the buffer (default: 0).
     */
    void BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const VkBuffer p_Buffer,
                            const VkDeviceSize p_Offset = 0) const noexcept
    {
        vkCmdBindVertexBuffers(p_CommandBuffer, 0, 1, &p_Buffer, &p_Offset);
    }

    VkBuffer GetBuffer() const noexcept
    {
        return m_Buffer.GetBuffer();
    }
    explicit(false) operator const Buffer &() const noexcept
    {
        return m_Buffer;
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
    Buffer m_Buffer{};
};
} // namespace VKit