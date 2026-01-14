#pragma once

#ifndef VKIT_ENABLE_DEVICE_BUFFER
#    error                                                                                                             \
        "[VULKIT][DEVICE-BUFFER] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_DEVICE_BUFFER"
#endif

#include "vkit/device/logical_device.hpp"
#include "vkit/memory/allocator.hpp"
#include "tkit/container/span.hpp"

namespace VKit
{
class CommandPool;
class DeviceImage;

using DeviceBufferFlags = u16;
enum DeviceBufferFlagBits : DeviceBufferFlags
{
    DeviceBufferFlag_DeviceLocal = 1 << 0,
    DeviceBufferFlag_HostVisible = 1 << 1,
    DeviceBufferFlag_Source = 1 << 2,
    DeviceBufferFlag_Destination = 1 << 3,
    DeviceBufferFlag_Staging = 1 << 4,
    DeviceBufferFlag_Vertex = 1 << 5,
    DeviceBufferFlag_Index = 1 << 6,
    DeviceBufferFlag_Storage = 1 << 7,
    DeviceBufferFlag_HostMapped = 1 << 8,
    DeviceBufferFlag_HostRandomAccess = 1 << 9,
};

class DeviceBuffer
{
  public:
    class Builder
    {
      public:
        Builder(const ProxyDevice &p_Device, VmaAllocator p_Allocator, DeviceBufferFlags p_Flags = 0);

        VKIT_NO_DISCARD Result<DeviceBuffer> Build() const;

        Builder &SetSize(VkDeviceSize p_Size);
        Builder &SetSize(VkDeviceSize p_InstanceCount, VkDeviceSize p_InstanceSize);
        template <typename T> Builder &SetSize(const VkDeviceSize p_InstanceCount)
        {
            return SetSize(p_InstanceCount, sizeof(T));
        }
        Builder &SetUsage(VkBufferUsageFlags p_Flags);
        Builder &SetSharingMode(VkSharingMode p_Mode);
        Builder &SetAllocationCreateInfo(const VmaAllocationCreateInfo &p_Info);
        Builder &SetPerInstanceMinimumAlignment(VkDeviceSize p_Alignment);

        Builder &AddFamilyIndex(u32 p_Index);
        template <typename T> Builder &AddNext(const T *p_Next)
        {
            const void *next = m_BufferInfo.pNext;
            p_Next->pNext = next;
            m_BufferInfo.pNext = p_Next;
        }

        const VkBufferCreateInfo &GetBufferInfo() const;

      private:
        ProxyDevice m_Device;
        VmaAllocator m_Allocator;
        VkDeviceSize m_InstanceCount = 0;
        VkDeviceSize m_InstanceSize = 0;
        VkBufferCreateInfo m_BufferInfo{};
        VmaAllocationCreateInfo m_AllocationInfo{};
        VkDeviceSize m_PerInstanceMinimumAlignment = 1;
        DeviceBufferFlags m_Flags;
        TKit::Array8<u32> m_FamilyIndices{};
    };

    struct Info
    {
        VmaAllocator Allocator;
        VmaAllocation Allocation;

        VkDeviceSize InstanceSize;
        VkDeviceSize InstanceCount;
        VkDeviceSize InstanceAlignedSize;
        VkDeviceSize Size;
        DeviceBufferFlags Flags;
    };

    DeviceBuffer() = default;

    DeviceBuffer(const ProxyDevice &p_Device, const VkBuffer p_Buffer, const Info &p_Info, void *p_MappedData)
        : m_Device(p_Device), m_Data(p_MappedData), m_Buffer(p_Buffer), m_Info(p_Info)
    {
    }

    void Destroy();

    VKIT_NO_DISCARD Result<> Map();
    void Unmap();

    bool IsMapped() const
    {
        return m_Data != nullptr;
    }

    const void *ReadAt(const u32 p_Index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");
        return static_cast<std::byte *>(m_Data) + m_Info.InstanceAlignedSize * p_Index;
    }
    void *ReadAt(const u32 p_Index)
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");
        return static_cast<std::byte *>(m_Data) + m_Info.InstanceAlignedSize * p_Index;
    }

    void Write(const void *p_Data, const VkBufferCopy &p_Copy);
    void WriteAt(u32 p_Index, const void *p_Data);

    void CopyFromBuffer(VkCommandBuffer p_CommandBuffer, const DeviceBuffer &p_Source,
                        TKit::Span<const VkBufferCopy> p_Copy);
    void CopyFromImage(VkCommandBuffer p_CommandBuffer, const DeviceImage &p_Source,
                       TKit::Span<const VkBufferImageCopy> p_Copy);

#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_copy_commands2)
    void CopyFromBuffer2(VkCommandBuffer p_CommandBuffer, const DeviceBuffer &p_Source,
                         TKit::Span<const VkBufferCopy2KHR> p_Copy, const void *p_Next = nullptr);
    void CopyFromImage2(VkCommandBuffer p_CommandBuffer, const DeviceImage &p_Source,
                        TKit::Span<const VkBufferImageCopy2KHR> p_Copy, const void *p_Next = nullptr);

#endif

    VKIT_NO_DISCARD Result<> CopyFromBuffer(CommandPool &p_Pool, VkQueue p_Queue, const DeviceBuffer &p_Source,
                                            TKit::Span<const VkBufferCopy> p_Copy);
    VKIT_NO_DISCARD Result<> CopyFromImage(CommandPool &p_Pool, VkQueue p_Queue, const DeviceImage &p_Source,
                                           TKit::Span<const VkBufferImageCopy> p_Copy);

    VKIT_NO_DISCARD Result<> UploadFromHost(CommandPool &p_Pool, const VkQueue p_Queue, const void *p_Data,
                                            const VkBufferCopy &p_Copy);

    const void *GetData() const
    {
        return m_Data;
    }
    void *GetData()
    {
        return m_Data;
    }

    VKIT_NO_DISCARD Result<> Flush(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0);

    VKIT_NO_DISCARD Result<> FlushAt(u32 p_Index);

    VKIT_NO_DISCARD Result<> Invalidate(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0);
    VKIT_NO_DISCARD Result<> InvalidateAt(u32 p_Index);

    template <typename Index> void BindAsIndexBuffer(VkCommandBuffer p_CommandBuffer, VkDeviceSize p_Offset = 0) const
    {
        static_assert(std::is_same_v<Index, u8> || std::is_same_v<Index, u16> || std::is_same_v<Index, u32>,
                      "[VULKIT][DEVICE-BUFFER] Index type must be u8, u16 or u32");
        if constexpr (std::is_same_v<Index, u8>)
            m_Device.Table->CmdBindIndexBuffer(p_CommandBuffer, m_Buffer, p_Offset, VK_INDEX_TYPE_UINT8_EXT);
        else if constexpr (std::is_same_v<Index, u16>)
            m_Device.Table->CmdBindIndexBuffer(p_CommandBuffer, m_Buffer, p_Offset, VK_INDEX_TYPE_UINT16);
        else if constexpr (std::is_same_v<Index, u32>)
            m_Device.Table->CmdBindIndexBuffer(p_CommandBuffer, m_Buffer, p_Offset, VK_INDEX_TYPE_UINT32);
    }
    void BindAsVertexBuffer(VkCommandBuffer p_CommandBuffer, VkDeviceSize p_Offset = 0) const;

    static void BindAsVertexBuffer(const ProxyDevice &p_Device, VkCommandBuffer p_CommandBuffer,
                                   TKit::Span<const VkBuffer> p_Buffers, u32 p_FirstBinding = 0,
                                   TKit::Span<const VkDeviceSize> p_Offsets = {});

    VkDescriptorBufferInfo CreateDescriptorInfo(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0) const;
    VkDescriptorBufferInfo CreateDescriptorInfoAt(u32 p_Index) const;

    const ProxyDevice &GetDevice() const
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
    ProxyDevice m_Device{};
    void *m_Data = nullptr;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
