#pragma once

#include "vkit/backend/system.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/core/vma.hpp"

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
    static Result<Buffer> Create(const Specs &p_Specs) noexcept;

    Buffer() noexcept = default;
    Buffer(VkBuffer p_Buffer, const Info &p_Info) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    void Map() noexcept;
    void Unmap() noexcept;

    /**
     * @brief Writes data to the buffer, up to the buffer size.
     *
     * The buffer must be mapped before calling this method.
     * Be very mindful of the alignment requirements of the buffer.
     *
     * @param p_Data A pointer to the data to write.
     */
    void Write(const void *p_Data) noexcept;

    /**
     * @brief Writes data to the buffer, up to the specified size, which must not exceed the buffer's.
     *
     * The buffer must be mapped before calling this method.
     * Be very mindful of the alignment requirements of the buffer.
     *
     * @param p_Data A pointer to the data to write.
     * @param p_Size The size of the data to write.
     */
    void Write(const void *p_Data, VkDeviceSize p_Size) noexcept;

    /**
     * @brief Writes data to the buffer, offsetted and up to the specified size, which must not exceed the buffer's.
     *
     * The buffer must be mapped before calling this method.
     * Be very mindful of the alignment requirements of the buffer.
     *
     * @param p_Data A pointer to the data to write.
     * @param p_Size The size of the data to write.
     * @param p_Offset The offset within the buffer to start writing.
     */
    void Write(const void *p_Data, VkDeviceSize p_Size, VkDeviceSize p_Offset) noexcept;

    /**
     * @brief Writes data to the buffer at the specified index.
     *
     * The buffer must be mapped before calling this method.
     *
     * Automatically handles alignment requirements.
     *
     * @param p_Index The index of the buffer instance to write to.
     * @param p_Data A pointer to the data to write.
     */
    void WriteAt(u32 p_Index, const void *p_Data) noexcept;

    /**
     * @brief Flushes a range of the buffer's memory to ensure visibility to the device.
     *
     * Ensures that any changes made to the buffer's mapped memory are visible to the GPU.
     *
     * @param p_Size The size of the memory range to flush (default: entire buffer).
     * @param p_Offset The offset within the buffer to start flushing (default: 0).
     */
    void Flush(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0) noexcept;

    /**
     * @brief Flushes a range of the buffer's memory at the specified index.
     *
     * Ensures that any changes made to the buffer's mapped memory are visible to the GPU.
     *
     * @param p_Index The index of the buffer instance to flush.
     */
    void FlushAt(u32 p_Index) noexcept;

    void Invalidate(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0) noexcept;
    void InvalidateAt(u32 p_Index) noexcept;

    VkDescriptorBufferInfo GetDescriptorInfo(VkDeviceSize p_Size = VK_WHOLE_SIZE,
                                             VkDeviceSize p_Offset = 0) const noexcept;
    VkDescriptorBufferInfo GetDescriptorInfoAt(u32 p_Index) const noexcept;

    void *GetData() const noexcept;
    void *ReadAt(u32 p_Index) const noexcept;

    /**
     * @brief Copies data from another buffer into this buffer.
     *
     * Uses a command pool and queue to perform the buffer-to-buffer copy operation.
     *
     * @param p_Source The source buffer to copy from.
     * @param p_Pool The command pool to allocate the copy command.
     * @param p_Queue The queue to submit the copy command.
     * @return A VulkanResult indicating success or failure.
     */
    VulkanResult DeviceCopy(const Buffer &p_Source, CommandPool &p_Pool, VkQueue p_Queue) noexcept;

    VkBuffer GetBuffer() const noexcept;
    explicit(false) operator VkBuffer() const noexcept;
    explicit(false) operator bool() const noexcept;

    const Info &GetInfo() const noexcept;

  private:
    void *m_Data = nullptr;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit