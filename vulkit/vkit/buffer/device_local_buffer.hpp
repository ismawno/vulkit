#pragma once

#include "vkit/buffer/buffer.hpp"
#include "vkit/rendering/command_pool.hpp"

namespace VKit
{
// considering adding Map/Unmap. Intel GPUs may allow it?

/**
 * @brief Represents a buffer stored in device-local memory.
 *
 * Manages Vulkan buffers optimized for GPU access, including methods for
 * creation, destruction, and binding as vertex or index buffers. Provides
 * specialized creation options for vertex, index, uniform, and storage buffers.
 *
 * @tparam T The type of data stored in the buffer.
 */
template <typename T>
    requires(std::is_trivially_destructible_v<T> && std::is_trivially_copyable_v<T>)
class DeviceLocalBuffer
{
  public:
    // To create an empty buffer, set data to Span{nullptr, desiredSize}. Pretty dodgy, but will work
    struct Specs
    {
        VmaAllocator Allocator = VK_NULL_HANDLE;
        TKit::Span<const T> Data{};
        VkBufferUsageFlags Usage;
        CommandPool *CommandPool = nullptr;
        VkQueue Queue = VK_NULL_HANDLE;
        VkDeviceSize PerInstanceMinimumAlignment = 1;
        VmaAllocationCreateFlags AllocationFlags = 0;
    };

    /**
     * @brief Creates a device-local buffer with the specified settings.
     *
     * Uses a staging buffer to upload data to the GPU. The data is stored
     * in device-local memory for optimal GPU access.
     *
     * @param p_Device The device to be used.
     * @param p_Specs The specifications for the buffer.
     * @return A `Result` containing the created `DeviceLocalBuffer` or an error.
     */
    static Result<DeviceLocalBuffer> Create(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs,
                                            const VkBufferUsageFlags p_Usage)
    {
        Buffer::Specs specs{};
        specs.Allocator = p_Specs.Allocator;
        specs.InstanceCount = p_Specs.Data.GetSize();
        specs.InstanceSize = sizeof(T);
        specs.Usage = p_Specs.Usage | p_Usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        specs.AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        specs.AllocationInfo.preferredFlags = 0;
        specs.AllocationInfo.flags = p_Specs.AllocationFlags;
        specs.PerInstanceMinimumAlignment = p_Specs.PerInstanceMinimumAlignment;

        auto result1 = Buffer::Create(p_Device, specs);
        if (!result1)
            return Result<DeviceLocalBuffer>::Error(result1.GetError().ErrorCode,
                                                    "Failed to create main device buffer");

        Buffer &buffer = result1.GetValue();
        if (!p_Specs.Data)
            return Result<DeviceLocalBuffer>::Ok(buffer);

        Buffer::Specs stagingSpecs = specs;
        stagingSpecs.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingSpecs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingSpecs.AllocationInfo.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        auto result2 = Buffer::Create(p_Device, stagingSpecs);
        if (!result2)
            return Result<DeviceLocalBuffer>::Error(result2.GetError(), "Failed to create staging buffer");

        Buffer &stagingBuffer = result2.GetValue();
        stagingBuffer.Write(p_Specs.Data.GetData());

        const auto result3 = buffer.DeviceCopy(stagingBuffer, *p_Specs.CommandPool, p_Specs.Queue);
        stagingBuffer.Destroy();
        if (!result3)
            return Result<DeviceLocalBuffer>::Error(result3.GetError(), "Failed to copy data to main buffer");

        return Result<DeviceLocalBuffer>::Ok(buffer);
    }

    /**
     * @brief Creates a device-local vertex buffer.
     *
     * Configures the buffer for vertex data and uploads the provided data
     * to device-local memory.
     *
     * @param p_Device The device to be used.
     * @param p_Specs The specifications for the vertex buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<DeviceLocalBuffer> CreateVertexBuffer(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs)
    {
        return Create(p_Device, p_Specs, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }

    /**
     * @brief Creates a device-local index buffer.
     *
     * Configures the buffer for index data and uploads the provided data
     * to device-local memory.
     *
     * @param p_Device The device to be used.
     * @param p_Specs The specifications for the index buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<DeviceLocalBuffer> CreateIndexBuffer(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs)
    {
        return Create(p_Device, p_Specs, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }

    /**
     * @brief Creates a device-local uniform buffer with the specified alignment.
     *
     * Configures the buffer for uniform data and uploads the provided data
     * to device-local memory.
     *
     * @param p_Device The device to be used.
     * @param p_Specs The specifications for the uniform buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<DeviceLocalBuffer> CreateUniformBuffer(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs)
    {
        return Create(p_Device, p_Specs, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    /**
     * @brief Creates a device-local storage buffer with the specified alignment.
     *
     * Configures the buffer for storage data and uploads the provided data
     * to device-local memory.
     *
     * @param p_Device The device to be used.
     * @param p_Specs The specifications for the storage buffer.
     * @return A `Result` containing the created `HostVisibleBuffer` or an error.
     */
    static Result<DeviceLocalBuffer> CreateStorageBuffer(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs)
    {
        return Create(p_Device, p_Specs, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }

    DeviceLocalBuffer() = default;
    DeviceLocalBuffer(const Buffer &p_Buffer) : m_Buffer(p_Buffer)
    {
    }

    void Destroy()
    {
        m_Buffer.Destroy();
    }
    void SubmitForDeletion(DeletionQueue &p_Queue) const
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
    void BindAsIndexBuffer(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset = 0) const
    {
        m_Buffer.BindAsIndexBuffer<T>(p_CommandBuffer, p_Offset);
    }

    /**
     * @brief Binds the buffer as a vertex buffer to a command buffer.
     *
     * @param p_CommandBuffer The command buffer to bind the vertex buffer to.
     * @param p_Offset The offset within the buffer (default: 0).
     */
    void BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset = 0) const
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
    static void BindAsVertexBuffer(const LogicalDevice::Proxy &p_Device, const VkCommandBuffer p_CommandBuffer,
                                   const TKit::Span<const VkBuffer> p_Buffers, const u32 p_FirstBinding = 0,
                                   const TKit::Span<const VkDeviceSize> p_Offsets = {})
    {
        Buffer::BindAsVertexBuffer(p_Device, p_CommandBuffer, p_Buffers, p_FirstBinding, p_Offsets);
    }

    /**
     * @brief Binds a buffer as a vertex buffer to a command buffer.
     *
     * @param p_CommandBuffer The command buffer to bind the vertex buffer to.
     * @param p_Offset The offset within the buffer (default: 0).
     */
    void BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const VkBuffer p_Buffer,
                            const VkDeviceSize p_Offset = 0) const
    {
        m_Buffer.BindAsVertexBuffer(p_CommandBuffer, p_Buffer, p_Offset);
    }

    VkDescriptorBufferInfo GetDescriptorInfo() const
    {
        return m_Buffer.GetDescriptorInfo();
    }
    VkDescriptorBufferInfo GetDescriptorInfoAt(const u32 p_Index) const
    {
        return m_Buffer.GetDescriptorInfoAt(p_Index);
    }
    const LogicalDevice::Proxy &GetDevice() const
    {
        return m_Buffer.GetDevice();
    }
    VkBuffer GetHandle() const
    {
        return m_Buffer.GetHandle();
    }
    operator const Buffer &() const
    {
        return m_Buffer;
    }
    operator VkBuffer() const
    {
        return m_Buffer.GetHandle();
    }
    operator bool() const
    {
        return static_cast<bool>(m_Buffer);
    }

    const Buffer::Info &GetInfo() const
    {
        return m_Buffer.GetInfo();
    }

  private:
    Buffer m_Buffer{};
};
} // namespace VKit
