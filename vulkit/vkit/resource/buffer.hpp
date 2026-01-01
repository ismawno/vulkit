#pragma once

#ifndef VKIT_ENABLE_BUFFER
#    error "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_BUFFER"
#endif

#include "vkit/device/logical_device.hpp"
#include "vkit/memory/allocator.hpp"
#include "vkit/resource/utils.hpp"
#include "tkit/container/span.hpp"

namespace VKit
{
class CommandPool;
class Image;

using BufferFlags = u16;
enum BufferFlagBits : BufferFlags
{
    BufferFlag_DeviceLocal = 1 << 0,
    BufferFlag_HostVisible = 1 << 1,
    BufferFlag_Source = 1 << 2,
    BufferFlag_Destination = 1 << 3,
    BufferFlag_Staging = 1 << 4,
    BufferFlag_Vertex = 1 << 5,
    BufferFlag_Index = 1 << 6,
    BufferFlag_Storage = 1 << 7,
    BufferFlag_HostMapped = 1 << 8,
    BufferFlag_HostRandomAccess = 1 << 9,
};

class VKIT_API Buffer
{
  public:
    class Builder
    {
      public:
        Builder(const LogicalDevice::Proxy &p_Device, VmaAllocator p_Allocator, BufferFlags p_Flags = 0);

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
        BufferFlags m_Flags;
    };

    struct Info
    {
        VmaAllocator Allocator;
        VmaAllocation Allocation;

        VkDeviceSize InstanceSize;
        VkDeviceSize InstanceCount;
        VkDeviceSize InstanceAlignedSize;
        VkDeviceSize Size;
        BufferFlags Flags;
    };

    Buffer() = default;

    Buffer(const LogicalDevice::Proxy &p_Device, const VkBuffer p_Buffer, const Info &p_Info, void *p_MappedData)
        : m_Device(p_Device), m_Data(p_MappedData), m_Buffer(p_Buffer), m_Info(p_Info)
    {
    }

    void Destroy();

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

    void Write(const void *p_Data, BufferCopy p_Info = {});

    template <typename T> void Write(const TKit::Span<const T> p_Data, const BufferCopy &p_Info = {})
    {
        const VkDeviceSize size = p_Info.Size == VK_WHOLE_SIZE
                                      ? (p_Data.GetSize() * sizeof(T) - p_Info.SrcOffset * sizeof(T))
                                      : (p_Info.Size * sizeof(T));
        Write(p_Data.GetData(),
              {.Size = size, .SrcOffset = p_Info.SrcOffset * sizeof(T), .DstOffset = p_Info.DstOffset * sizeof(T)});
    }

    void WriteAt(u32 p_Index, const void *p_Data);

    void CopyFromBuffer(VkCommandBuffer p_CommandBuffer, const Buffer &p_Source, const BufferCopy &p_Info);

    Result<> CopyFromBuffer(CommandPool &p_Pool, VkQueue p_Queue, const Buffer &p_Source,
                            const BufferCopy &p_Info = {});

    void CopyFromImage(VkCommandBuffer p_CommandBuffer, const Image &p_Source, const BufferImageCopy &p_Info);

    Result<> CopyFromImage(CommandPool &p_Pool, VkQueue p_Queue, const Image &p_Source,
                           const BufferImageCopy &p_Info = {});

    Result<> UploadFromHost(CommandPool &p_Pool, const VkQueue p_Queue, const void *p_Data,
                            const BufferCopy &p_Info = {});
    template <typename T>
    Result<> UploadFromHost(CommandPool &p_Pool, VkQueue p_Queue, const TKit::Span<const T> p_Data,
                            const BufferCopy &p_Info = {})
    {
        const VkDeviceSize size = p_Info.Size == VK_WHOLE_SIZE
                                      ? (p_Data.GetSize() * sizeof(T) - p_Info.SrcOffset * sizeof(T))
                                      : (p_Info.Size * sizeof(T));

        return UploadFromHost(
            p_Pool, p_Queue, p_Data.GetData(),
            {.Size = size, .SrcOffset = p_Info.SrcOffset * sizeof(T), .DstOffset = p_Info.DstOffset * sizeof(T)});
    }

    void *GetData() const
    {
        return m_Data;
    }

    Result<> Flush(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0);

    Result<> FlushAt(u32 p_Index);

    Result<> Invalidate(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0);
    Result<> InvalidateAt(u32 p_Index);

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
    void BindAsVertexBuffer(VkCommandBuffer p_CommandBuffer, VkDeviceSize p_Offset = 0) const;

    static void BindAsVertexBuffer(const LogicalDevice::Proxy &p_Device, VkCommandBuffer p_CommandBuffer,
                                   TKit::Span<const VkBuffer> p_Buffers, u32 p_FirstBinding = 0,
                                   TKit::Span<const VkDeviceSize> p_Offsets = {});

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
