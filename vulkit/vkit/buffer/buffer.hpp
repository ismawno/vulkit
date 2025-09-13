#pragma once

#ifndef VKIT_ENABLE_BUFFER
#    error "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_BUFFER"
#endif

#include "vkit/vulkan/logical_device.hpp"
#include "vkit/vulkan/allocator.hpp"
#include "tkit/container/span.hpp"

#include <vulkan/vulkan.h>

// User may not use mutable buffer methods if the buffer is not/cannot be mapped

namespace VKit
{
class CommandPool;
/**
 * @brief Manages a Vulkan buffer and its associated memory.
 *
 * Provides methods for buffer creation, memory mapping, data writing, flushing, and invalidation.
 * Supports descriptor info retrieval and buffer-to-buffer copy operations.
 */
class VKIT_API Buffer
{
  public:
    /**
     * @brief Specifications for creating a Vulkan buffer.
     *
     * @note: The `PerInstanceMinimumAlignment` is only needed when binding, flushing or invalidating specific parts of
     * the buffer, as the offsets used have to be aligned to a certain offset, provided by the device. If the buffer is
     * going to be operated on as a whole, this must be set to 1.
     */

    struct Specs
    {
        VmaAllocator Allocator = VK_NULL_HANDLE;
        VkDeviceSize InstanceCount;
        VkDeviceSize InstanceSize;
        VkBufferUsageFlags Usage;
        VmaAllocationCreateInfo AllocationInfo;
        VkDeviceSize PerInstanceMinimumAlignment = 1;
    };

    struct Info
    {
        VmaAllocator Allocator;
        VmaAllocation Allocation;

        VkDeviceSize InstanceSize;
        VkDeviceSize InstanceCount;
        VkDeviceSize InstanceAlignedSize;
        VkDeviceSize Size;
    };

    /**
     * @brief Creates a Vulkan buffer based on the provided specifications.
     *
     * Initializes the buffer with the specified size, usage, and memory allocation settings.
     *
     * @param p_Specs The specifications for the buffer.
     * @return A `Result` containing the created `Buffer` or an error.
     */
    static Result<Buffer> Create(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs);

    Buffer() = default;
    Buffer(const LogicalDevice::Proxy &p_Device, VkBuffer p_Buffer, const Info &p_Info, void *p_MappedData);

    void Destroy();
    void SubmitForDeletion(DeletionQueue &p_Queue) const;

    void Map();
    void Unmap();
    bool IsMapped() const;

    /**
     * @brief Writes data to the buffer, up to the buffer size.
     *
     * The buffer must be mapped before calling this method. It will automatically flush the memory if needed.
     *
     * @param p_Data A pointer to the data to write.
     */
    void Write(const void *p_Data);

    /**
     * @brief Writes data to the buffer, offsetted and up to the specified size, which must not exceed the buffer's.
     *
     * The buffer must be mapped before calling this method. It will automatically flush the memory if needed.
     *
     * @param p_Data A pointer to the data to write.
     * @param p_Size The size of the data to write.
     * @param p_Offset The offset within the buffer to start writing.
     */
    void Write(const void *p_Data, VkDeviceSize p_Size, VkDeviceSize p_Offset = 0);

    /**
     * @brief Writes data to the buffer at the specified index.
     *
     * The buffer must be mapped before calling this method. It will automatically flush the memory if needed.
     *
     * @param p_Index The index of the buffer instance to write to.
     * @param p_Data A pointer to the data to write.
     */
    void WriteAt(u32 p_Index, const void *p_Data);

    /**
     * @brief Flushes a range of the buffer's memory to ensure visibility to the device.
     *
     * Ensures that any changes made to the buffer's mapped memory are visible to the GPU.
     *
     * @param p_Size The size of the memory range to flush (default: entire buffer).
     * @param p_Offset The offset within the buffer to start flushing (default: 0).
     */
    void Flush(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0);

    /**
     * @brief Flushes a range of the buffer's memory at the specified index.
     *
     * Ensures that any changes made to the buffer's mapped memory are visible to the GPU.
     *
     * @param p_Index The index of the buffer instance to flush.
     */
    void FlushAt(u32 p_Index);

    void Invalidate(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0);
    void InvalidateAt(u32 p_Index);

    /**
     * @brief Binds the buffer as an index buffer to a command buffer.
     *
     * Automatically detects the index type (`u8`, `u16`, or `u32`) based on the buffer's template parameter.
     *
     * @param p_CommandBuffer The command buffer to bind the index buffer to.
     * @param p_Offset The offset within the buffer (default: 0).
     */
    template <typename Index>
    void BindAsIndexBuffer(VkCommandBuffer p_CommandBuffer, VkDeviceSize p_Offset = 0) const
    {
        static_assert(std::is_same_v<Index, u8> || std::is_same_v<Index, u16> || std::is_same_v<Index, u32>,
                      "[VULKIT] Index type must be u8, u16 or u32");
        if constexpr (std::is_same_v<Index, u8>)
            m_Device.Table->CmdBindIndexBuffer(p_CommandBuffer, m_Buffer, p_Offset, VK_INDEX_TYPE_UINT8_EXT);
        else if constexpr (std::is_same_v<Index, u16>)
            m_Device.Table->CmdBindIndexBuffer(p_CommandBuffer, m_Buffer, p_Offset, VK_INDEX_TYPE_UINT16);
        else if constexpr (std::is_same_v<Index, u32>)
            m_Device.Table->CmdBindIndexBuffer(p_CommandBuffer, m_Buffer, p_Offset, VK_INDEX_TYPE_UINT32);
    }
    /**
     * @brief Binds the buffer as a vertex buffer to a command buffer.
     *
     * @param p_CommandBuffer The command buffer to bind the vertex buffer to.
     * @param p_Offset The offset within the buffer (default: 0).
     */
    void BindAsVertexBuffer(VkCommandBuffer p_CommandBuffer, VkDeviceSize p_Offset = 0) const;

    /**
     * @brief Binds multiple buffers as vertex buffers to a command buffer.
     *
     * @param p_Device The logical device to use for binding.
     * @param p_CommandBuffer The command buffer to bind the vertex buffers to.
     * @param p_Buffers A span containing the buffers to bind.
     * @param p_Offsets A span containing the offsets within the buffers (default: 0).
     */
    static void BindAsVertexBuffer(const LogicalDevice::Proxy &p_Device, VkCommandBuffer p_CommandBuffer,
                                   TKit::Span<const VkBuffer> p_Buffers, u32 p_FirstBinding = 0,
                                   TKit::Span<const VkDeviceSize> p_Offsets = {});

    /**
     * @brief Binds a buffer as a vertex buffer to a command buffer.
     *
     * @param p_CommandBuffer The command buffer to bind the vertex buffer to.
     * @param p_Offset The offset within the buffer (default: 0).
     */
    void BindAsVertexBuffer(VkCommandBuffer p_CommandBuffer, VkBuffer p_Buffer,
                            VkDeviceSize p_Offset = 0) const;

    VkDescriptorBufferInfo GetDescriptorInfo(VkDeviceSize p_Size = VK_WHOLE_SIZE,
                                             VkDeviceSize p_Offset = 0) const;
    VkDescriptorBufferInfo GetDescriptorInfoAt(u32 p_Index) const;

    void *GetData() const;
    void *ReadAt(u32 p_Index) const;

    /**
     * @brief Copies data from another buffer into this buffer.
     *
     * Uses a command pool and queue to perform the buffer-to-buffer copy operation.
     *
     * @param p_Source The source buffer to copy from.
     * @param p_Pool The command pool to allocate the copy command.
     * @param p_Queue The queue to submit the copy command.
     * @return A `Result` indicating success or failure.
     */
    Result<> DeviceCopy(const Buffer &p_Source, CommandPool &p_Pool, VkQueue p_Queue);

    const LogicalDevice::Proxy &GetDevice() const;
    VkBuffer GetHandle() const;

    operator VkBuffer() const;
    operator bool() const;

    const Info &GetInfo() const;

  private:
    LogicalDevice::Proxy m_Device{};
    void *m_Data = nullptr;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
