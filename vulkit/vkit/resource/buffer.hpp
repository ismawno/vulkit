#pragma once

#ifndef VKIT_ENABLE_BUFFER
#    error "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_BUFFER"
#endif

#include "vkit/vulkan/logical_device.hpp"
#include "vkit/vulkan/allocator.hpp"
#include "vkit/resource/utils.hpp"
#include "tkit/container/span.hpp"

namespace VKit
{
class CommandPool;
class Image;

/**
 * @brief Manages a Vulkan buffer and its associated memory.
 *
 * Provides methods for buffer creation, memory mapping, data writing, flushing, and invalidation.
 * Supports descriptor info retrieval and buffer-to-buffer copy operations.
 */
class VKIT_API Buffer
{
  public:
    using Flags = u16;
    enum FlagBits
    {
        Flag_None = 0,
        Flag_DeviceLocal = 1 << 0,
        Flag_HostVisible = 1 << 1,
        Flag_Source = 1 << 2,
        Flag_Destination = 1 << 3,
        Flag_StagingBuffer = 1 << 4,
        Flag_VertexBuffer = 1 << 4,
        Flag_IndexBuffer = 1 << 5,
        Flag_StorageBuffer = 1 << 6,
        Flag_Mapped = 1 << 7,
        Flag_RandomAccess = 1 << 8,
    };

    class Builder
    {
      public:
        Builder(const LogicalDevice::Proxy &p_Device, VmaAllocator p_Allocator, Flags p_Flags = 0);

        Result<Buffer> Build() const;

        Builder &SetSize(VkDeviceSize p_Size);
        Builder &SetSize(VkDeviceSize p_InstanceCount, VkDeviceSize p_InstanceSize);
        template <typename T> Builder &SetSize(const VkDeviceSize p_InstanceCount)
        {
            return SetSize(p_InstanceCount, sizeof(T));
        }
        Builder &SetUsage(VkBufferUsageFlags p_Flags);
        Builder &SetAllocationCreateInfo(const VmaAllocationCreateInfo &p_Info);
        Builder &SetPerInstanceMinimumAlignment(VkDeviceSize p_Alignment);

      private:
        LogicalDevice::Proxy m_Device;
        VmaAllocator m_Allocator;
        VkDeviceSize m_InstanceCount = 0;
        VkDeviceSize m_InstanceSize = 0;
        VkBufferUsageFlags m_Usage = 0;
        VmaAllocationCreateInfo m_AllocationInfo{};
        VkDeviceSize m_PerInstanceMinimumAlignment = 1;
        Flags m_Flags;
    };

    struct Info
    {
        VmaAllocator Allocator;
        VmaAllocation Allocation;

        VkDeviceSize InstanceSize;
        VkDeviceSize InstanceCount;
        VkDeviceSize InstanceAlignedSize;
        VkDeviceSize Size;
        Flags Flags;
    };

    Buffer() = default;

    Buffer(const LogicalDevice::Proxy &p_Device, const VkBuffer p_Buffer, const Info &p_Info, void *p_MappedData)
        : m_Device(p_Device), m_Data(p_MappedData), m_Buffer(p_Buffer), m_Info(p_Info)
    {
    }

    void Destroy();
    void SubmitForDeletion(DeletionQueue &p_Queue) const;

    Result<> Map();
    void Unmap();

    bool IsMapped() const
    {
        return m_Data != nullptr;
    }

    void *ReadAt(const u32 p_Index) const
    {
        TKIT_ASSERT(p_Index < m_Info.InstanceCount, "[VULKIT] Index out of bounds");
        return static_cast<std::byte *>(m_Data) + m_Info.InstanceAlignedSize * p_Index;
    }

    /**
     * @brief Writes data to the buffer, offsetted and up to the specified size, which must not exceed the buffer's.
     *
     * The buffer must be mapped and host visible to call this method.
     *
     * @param p_Data A pointer to the data to write.
     * @param p_Info Information about the range of the copy.
     */
    void Write(const void *p_Data, BufferCopy p_Info = {});

    /**
     * @brief Writes data to the buffer from a span.
     *
     * The buffer must be mapped and host visible to call this method.
     *
     * @param p_Data A pointer to the data to write.
     */
    template <typename T> void Write(const TKit::Span<const T> p_Data)
    {
        Write(p_Data.GetData(), {.Size = p_Data.GetSize() * sizeof(T)});
    }

    /**
     * @brief Writes data to the buffer at the specified index.
     *
     * The buffer must be mapped and host visible to call this method.
     *
     * @param p_Index The index of the buffer instance to write to.
     * @param p_Data A pointer to the data to write.
     */
    void WriteAt(u32 p_Index, const void *p_Data);

    /**
     * @brief Copies data from another buffer into this buffer.
     *
     * Records the copy into a command buffer.
     *
     * @param p_CommandBuffer The command buffer to which the copy will be recorded.
     * @param p_Source The source buffer to copy from.
     * @param p_Info Information about the range of the copy.
     */
    void CopyFromBuffer(VkCommandBuffer p_CommandBuffer, const Buffer &p_Source, const BufferCopy &p_Info);

    /**
     * @brief Copies data from another buffer into this buffer.
     *
     * Uses a command pool and queue to perform the buffer-to-buffer copy operation.
     *
     * @param p_Pool The command pool to allocate the copy command.
     * @param p_Queue The queue to submit the copy command.
     * @param p_Source The source buffer to copy from.
     * @param p_Info Information about the range of the copy.
     * @return A `Result` indicating success or failure.
     */
    Result<> CopyFromBuffer(CommandPool &p_Pool, VkQueue p_Queue, const Buffer &p_Source,
                            const BufferCopy &p_Info = {});

    /**
     * @brief Copies data from an image into this buffer.
     *
     * Records the copy into a command buffer.
     *
     * @param p_CommandBuffer The command buffer to which the copy will be recorded.
     * @param p_Source The source image to copy from.
     * @param p_Info Information about the range of the copy.
     */
    void CopyFromImage(VkCommandBuffer p_CommandBuffer, const Image &p_Source, const BufferImageCopy &p_Info);

    /**
     * @brief Copies data from an image into this buffer.
     *
     * Records the copy into a command buffer.
     *
     * @param p_CommandBuffer The command buffer to which the copy will be recorded.
     * @param p_Source The source image to copy from.
     * @param p_Info Information about the range of the copy.
     * @return A `Result` indicating success or failure.
     */
    Result<> CopyFromImage(CommandPool &p_Pool, VkQueue p_Queue, const Image &p_Source,
                           const BufferImageCopy &p_Info = {});

    /**
     * @brief Uploads host data to the buffer, offsetted and up to the specified size, which must not exceed the
     * buffer's.
     *
     * This method is designed to be used for device local buffers.
     *
     * @param p_Pool The command pool to allocate the copy command.
     * @param p_Queue The queue to submit the copy command.
     * @param p_Data A pointer to the data to write.
     * @param p_Info Information about the range of the copy.
     * @return A `Result` indicating success or failure.
     */

    Result<> UploadFromHost(CommandPool &p_Pool, VkQueue p_Queue, const void *p_Data, const BufferCopy &p_Info = {});
    /**
     * @brief Writes data to the buffer from a span.
     *
     * The buffer must be mapped and host visible to call this method.
     *
     * @param p_Data A pointer to the data to write.
     */
    template <typename T>
    Result<> UploadFromHost(CommandPool &p_Pool, VkQueue p_Queue, const TKit::Span<const T> p_Data)
    {
        UploadFromHost(p_Pool, p_Queue, p_Data.GetData(), {.Size = p_Data.GetSize() * sizeof(T)});
    }

    void *GetData() const
    {
        return m_Data;
    }

    /**
     * @brief Flushes a range of the buffer's memory to ensure visibility to the device.
     *
     * Ensures that any changes made to the buffer's mapped memory are visible to the GPU.
     *
     * @param p_Size The size of the memory range to flush (default: entire buffer).
     * @param p_Offset The offset within the buffer to start flushing (default: 0).
     */
    Result<> Flush(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0);

    /**
     * @brief Flushes a range of the buffer's memory at the specified index.
     *
     * Ensures that any changes made to the buffer's mapped memory are visible to the GPU.
     *
     * @param p_Index The index of the buffer instance to flush.
     */
    Result<> FlushAt(u32 p_Index);

    Result<> Invalidate(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0);
    Result<> InvalidateAt(u32 p_Index);

    /**
     * @brief Binds the buffer as an index buffer to a command buffer.
     *
     * Automatically detects the index type (`u8`, `u16`, or `u32`) based on the buffer's template parameter.
     *
     * @param p_CommandBuffer The command buffer to bind the index buffer to.
     * @param p_Offset The offset within the buffer (default: 0).
     */
    template <typename Index> void BindAsIndexBuffer(VkCommandBuffer p_CommandBuffer, VkDeviceSize p_Offset = 0) const
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
    void BindAsVertexBuffer(VkCommandBuffer p_CommandBuffer, VkBuffer p_Buffer, VkDeviceSize p_Offset = 0) const;

    VkDescriptorBufferInfo GetDescriptorInfo(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0) const;
    VkDescriptorBufferInfo GetDescriptorInfoAt(u32 p_Index) const;

    const LogicalDevice::Proxy &GetDevice() const
    {
        return m_Device;
    }
    VkBuffer GetHandle() const
    {
        return m_Buffer;
    }
    operator VkBuffer() const
    {
        return m_Buffer;
    }
    operator bool() const
    {
        return m_Buffer != VK_NULL_HANDLE;
    }

    const Info &GetInfo() const
    {
        return m_Info;
    }

  private:
    LogicalDevice::Proxy m_Device{};
    void *m_Data = nullptr;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
