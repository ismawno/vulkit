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
template <typename T>
    requires(std::is_trivially_destructible_v<T> && std::is_trivially_copyable_v<T>)
class HostVisibleBuffer
{
  public:
    struct Specs
    {
        VmaAllocator Allocator = VK_NULL_HANDLE;
        VkDeviceSize Capacity;
        VkBufferUsageFlags Usage;
        VkDeviceSize PerInstanceMinimumAlignment = 1;
        VmaAllocationCreateFlags AllocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    };

    /**
     * @brief Creates a host-visible buffer with the specified settings.
     *
     * Allocates and maps a Vulkan buffer in host-visible memory based on the provided specifications.
     *
     * @param p_Specs The specifications for the buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<HostVisibleBuffer> Create(const Specs &p_Specs, const VkBufferUsageFlags p_Usage) noexcept
    {
        Buffer::Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.InstanceCount = p_Specs.Capacity;
        specs.InstanceSize = sizeof(T);
        specs.Usage = p_Usage;
        specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
        specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        specs.AllocationInfo.preferredFlags = 0;
        specs.AllocationInfo.flags = p_Specs.AllocationFlags | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        specs.PerInstanceMinimumAlignment = p_Specs.PerInstanceMinimumAlignment;

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
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<HostVisibleBuffer> CreateVertexBuffer(const Specs &p_Specs) noexcept
    {
        return Create(p_Specs, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }

    /**
     * @brief Creates a host-visible index buffer.
     *
     * Configures the buffer for index data and allocates memory in host-visible space.
     *
     * @param p_Specs The specifications for the index buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<HostVisibleBuffer> CreateIndexBuffer(const Specs &p_Specs) noexcept
    {
        return Create(p_Specs, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }

    /**
     * @brief Creates a host-visible uniform buffer with specified alignment.
     *
     * Configures the buffer for uniform data and allocates memory with the required alignment.
     *
     * @param p_Specs The specifications for the uniform buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<HostVisibleBuffer> CreateUniformBuffer(const Specs &p_Specs) noexcept
    {
        return Create(p_Specs, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    /**
     * @brief Creates a host-visible storage buffer with specified alignment.
     *
     * Configures the buffer for storage data and allocates memory with the required alignment.
     *
     * @param p_Specs The specifications for the storage buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<HostVisibleBuffer> CreateStorageBuffer(const Specs &p_Specs) noexcept
    {
        return Create(p_Specs, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }

    HostVisibleBuffer() noexcept = default;
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

    /**
     * @brief Writes data to the buffer, up to the buffer size.
     *
     * The buffer must be mapped before calling this method. It will automatically flush the memory if needed.
     *
     * @param p_Data A pointer to the data to write.
     */
    void Write(const T *p_Data) noexcept
    {
        m_Buffer.Write(p_Data);
    }

    /**
     * @brief Writes data to the buffer, offsetted and up to the specified size, which must not exceed the buffer's.
     *
     * The buffer must be mapped before calling this method. It will automatically flush the memory if needed.
     *
     * @param p_Data A pointer to the data to write.
     * @param p_Size The size of the data to write.
     * @param p_Offset The offset within the buffer to start writing.
     */
    void Write(const T *p_Data, const VkDeviceSize p_Size, const VkDeviceSize p_Offset = 0) noexcept
    {
        m_Buffer.Write(p_Data, p_Size, p_Offset);
    }

    /**
     * @brief Writes a span of data to the buffer.
     *
     * The buffer must be mapped before calling this method. It will automatically flush the memory if needed.
     *
     * @param p_Data A span containing the data to write.
     * @param p_Offset The offset within the buffer to start writing, not in bytes but in elements.
     */
    void Write(const TKit::Span<const T> p_Data, const usize p_Offset = 0) noexcept
    {
        m_Buffer.Write(p_Data.GetData(), p_Data.GetSize() * sizeof(T), p_Offset * sizeof(T));
    }

    /**
     * @brief Writes data to the buffer at the specified index.
     *
     * Copies the provided data into the buffer at the specified index.
     * The buffer must be mapped before calling this method. It will automatically flush the memory if needed.
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
     * Automatically detects the index type (`u8`, `u16`, or `u32`) based on the buffer's template parameter.
     *
     * @param p_CommandBuffer The command buffer to bind the index buffer to.
     * @param p_Offset The offset within the buffer (default: 0).
     */
    void BindAsIndexBuffer(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset = 0) const noexcept
    {
        m_Buffer.BindAsIndexBuffer<T>(p_CommandBuffer, p_Offset);
    }

    /**
     * @brief Binds the buffer as a vertex buffer to a command buffer.
     *
     * @param p_CommandBuffer The command buffer to bind the vertex buffer to.
     * @param p_Offset The offset within the buffer (default: 0).
     */
    void BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset = 0) const noexcept
    {
        m_Buffer.BindAsVertexBuffer(p_CommandBuffer, p_Offset);
    }

    /**
     * @brief Binds multiple buffers as vertex buffers to a command buffer.
     *
     * @param p_CommandBuffer The command buffer to bind the vertex buffers to.
     * @param p_Buffers A span containing the buffers to bind.
     * @param p_Offsets A span containing the offsets within the buffers (default: 0).
     */
    static void BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const TKit::Span<const VkBuffer> p_Buffers,
                                   const u32 p_FirstBinding = 0,
                                   const TKit::Span<const VkDeviceSize> p_Offsets = {}) noexcept
    {
        Buffer::BindAsVertexBuffer(p_CommandBuffer, p_Buffers, p_FirstBinding, p_Offsets);
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
        m_Buffer.BindAsVertexBuffer(p_CommandBuffer, p_Buffer, p_Offset);
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