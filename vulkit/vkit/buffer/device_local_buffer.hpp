#pragma once

#include "vkit/buffer/buffer.hpp"
#include "vkit/backend/command_pool.hpp"
#include "tkit/container/buffer.hpp"

namespace VKit
{
/**
 * @brief Represents a buffer stored in device-local memory.
 *
 * Manages Vulkan buffers optimized for GPU access, including methods for
 * creation, destruction, and binding as vertex or index buffers. Provides
 * specialized creation options for vertex, index, uniform, and storage buffers.
 *
 * @tparam T The type of data stored in the buffer.
 */
template <typename T> class DeviceLocalBuffer
{
  public:
    struct Specs
    {
        VmaAllocator Allocator = VK_NULL_HANDLE;
        const TKit::Buffer<T> *Data = nullptr;
        VkBufferUsageFlags Usage;
        CommandPool *CommandPool = nullptr;
        VkQueue Queue = VK_NULL_HANDLE;
        VkDeviceSize MinimumAlignment = 1;
        VmaAllocationCreateFlags AllocationFlags = 0;
    };

    struct SpecializedSpecs
    {
        VmaAllocator Allocator = VK_NULL_HANDLE;
        const TKit::Buffer<T> *Data = nullptr;
        CommandPool *CommandPool = nullptr;
        VkQueue Queue = VK_NULL_HANDLE;
        VmaAllocationCreateFlags AllocationFlags = 0;
    };

    using VertexSpecs = SpecializedSpecs;
    using IndexSpecs = SpecializedSpecs;
    using UniformSpecs = SpecializedSpecs;
    using StorageSpecs = SpecializedSpecs;

    /**
     * @brief Creates a device-local buffer with the specified settings.
     *
     * Uses a staging buffer to upload data to the GPU. The data is stored
     * in device-local memory for optimal GPU access.
     *
     * @param p_Specs The specifications for the buffer.
     * @return A `Result` containing the created `DeviceLocalBuffer` or an error.
     */
    static Result<DeviceLocalBuffer> Create(const Specs &p_Specs) noexcept
    {
        TKIT_ASSERT(p_Specs.Data->GetMinimumInstanceAlignment() == p_Specs.MinimumAlignment,
                    "[VULKIT] Provided buffer's minimum alignment does not match the specified minimum alignment");

        Buffer::Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.InstanceCount = p_Specs.Data->GetInstanceCount();
        specs.InstanceSize = sizeof(T);
        specs.Usage = p_Specs.Usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        specs.AllocationInfo.preferredFlags = 0;
        specs.AllocationInfo.flags = p_Specs.AllocationFlags;
        specs.MinimumAlignment = p_Specs.MinimumAlignment;

        auto result1 = Buffer::Create(specs);
        if (!result1)
            return Result<DeviceLocalBuffer>::Error(result1.GetError().Result, "Failed to create main device buffer");

        Buffer &buffer = result1.GetValue();

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
        stagingBuffer.Write(p_Specs.Data->GetData());
        stagingBuffer.Flush();
        stagingBuffer.Unmap();

        const auto result3 = buffer.DeviceCopy(stagingBuffer, *p_Specs.CommandPool, p_Specs.Queue);
        stagingBuffer.Destroy();
        if (!result3)
            return Result<DeviceLocalBuffer>::Error(result3.Result, "Failed to copy data to main buffer");

        return Result<DeviceLocalBuffer>::Ok(buffer);
    }

    /**
     * @brief Creates a device-local vertex buffer.
     *
     * Configures the buffer for vertex data and uploads the provided data
     * to device-local memory.
     *
     * @param p_Specs The specifications for the vertex buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<DeviceLocalBuffer> CreateVertexBuffer(const VertexSpecs &p_Specs) noexcept
    {
        Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.Data = p_Specs.Data;
        specs.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        specs.CommandPool = p_Specs.CommandPool;
        specs.Queue = p_Specs.Queue;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        return Create(specs);
    }

    /**
     * @brief Creates a device-local index buffer.
     *
     * Configures the buffer for index data and uploads the provided data
     * to device-local memory.
     *
     * @param p_Specs The specifications for the index buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<DeviceLocalBuffer> CreateIndexBuffer(const IndexSpecs &p_Specs) noexcept
    {
        Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.Data = p_Specs.Data;
        specs.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        specs.CommandPool = p_Specs.CommandPool;
        specs.Queue = p_Specs.Queue;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        return Create(specs);
    }

    /**
     * @brief Creates a device-local uniform buffer with the specified alignment.
     *
     * Configures the buffer for uniform data and uploads the provided data
     * to device-local memory.
     *
     * @param p_Specs The specifications for the uniform buffer.
     * @param p_Alignment The minimum alignment for the uniform buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<DeviceLocalBuffer> CreateUniformBuffer(const UniformSpecs &p_Specs,
                                                         const VkDeviceSize p_Alignment) noexcept
    {
        Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.Data = p_Specs.Data;
        specs.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        specs.CommandPool = p_Specs.CommandPool;
        specs.Queue = p_Specs.Queue;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        specs.MinimumAlignment = p_Alignment;
        return Create(specs);
    }

    /**
     * @brief Creates a device-local storage buffer with the specified alignment.
     *
     * Configures the buffer for storage data and uploads the provided data
     * to device-local memory.
     *
     * @param p_Specs The specifications for the storage buffer.
     * @param p_Alignment The minimum alignment for the storage buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<DeviceLocalBuffer> CreateStorageBuffer(const StorageSpecs &p_Specs,
                                                         const VkDeviceSize p_Alignment) noexcept
    {
        Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.Data = p_Specs.Data;
        specs.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        specs.CommandPool = p_Specs.CommandPool;
        specs.Queue = p_Specs.Queue;
        specs.AllocationFlags = p_Specs.AllocationFlags;
        specs.MinimumAlignment = p_Alignment;
        return Create(specs);
    }

    DeviceLocalBuffer() noexcept = default;
    explicit DeviceLocalBuffer(const Buffer &p_Buffer) noexcept : m_Buffer(p_Buffer)
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

    /**
     * @brief Binds the buffer as an index buffer to a command buffer.
     *
     * Automatically detects the index type (`u8`, `u16`, or `u32`) based on the buffer's template parameter.
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
    static void BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const TKit::Span<const VkBuffer> p_Buffers,
                                   const TKit::Span<const VkDeviceSize> p_Offsets = {}) noexcept
    {
        if (!p_Offsets.empty())
            vkCmdBindVertexBuffers(p_CommandBuffer, 0, p_Buffers.size(), p_Buffers.data(), p_Offsets.data());
        else
            vkCmdBindVertexBuffers(p_CommandBuffer, 0, p_Buffers.size(), p_Buffers.data(), nullptr);
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

    VkDescriptorBufferInfo GetDescriptorInfo() const noexcept
    {
        return m_Buffer.GetDescriptorInfo();
    }
    VkDescriptorBufferInfo GetDescriptorInfoAt(const u32 p_Index) const noexcept
    {
        return m_Buffer.GetDescriptorInfoAt(p_Index);
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