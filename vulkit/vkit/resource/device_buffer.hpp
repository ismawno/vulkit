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
        Builder(const ProxyDevice &device, VmaAllocator allocator, DeviceBufferFlags flags = 0);

        VKIT_NO_DISCARD Result<DeviceBuffer> Build() const;

        Builder &SetSize(VkDeviceSize size);
        Builder &SetSize(VkDeviceSize instanceCount, VkDeviceSize instanceSize);
        template <typename T> Builder &SetSize(const VkDeviceSize instanceCount)
        {
            return SetSize(instanceCount, sizeof(T));
        }
        Builder &SetUsage(VkBufferUsageFlags flags);
        Builder &SetSharingMode(VkSharingMode mode);
        Builder &SetAllocationCreateInfo(const VmaAllocationCreateInfo &info);
        Builder &SetPerInstanceMinimumAlignment(VkDeviceSize alignment);

        Builder &AddFamilyIndex(u32 index);
        Builder &SetNext(const void *next);

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
        TKit::TierArray<u32> m_FamilyIndices{};
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

    DeviceBuffer(const ProxyDevice &device, const VkBuffer buffer, const Info &info, void *mappedData)
        : m_Device(device), m_Data(mappedData), m_Buffer(buffer), m_Info(info)
    {
    }

    void Destroy();

    VKIT_NO_DISCARD Result<> Map();
    void Unmap();

    bool IsMapped() const
    {
        return m_Data != nullptr;
    }

    const void *ReadAt(const u32 index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");
        return static_cast<std::byte *>(m_Data) + m_Info.InstanceAlignedSize * index;
    }
    void *ReadAt(const u32 index)
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");
        return static_cast<std::byte *>(m_Data) + m_Info.InstanceAlignedSize * index;
    }

    void Write(const void *data, const VkBufferCopy &copy);
    void WriteAt(u32 index, const void *data);

    void CopyFromBuffer(VkCommandBuffer commandBuffer, const DeviceBuffer &source, TKit::Span<const VkBufferCopy> copy);
    void CopyFromImage(VkCommandBuffer commandBuffer, const DeviceImage &source,
                       TKit::Span<const VkBufferImageCopy> copy);

#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_copy_commands2)
    void CopyFromBuffer2(VkCommandBuffer commandBuffer, const DeviceBuffer &source,
                         TKit::Span<const VkBufferCopy2KHR> copy, const void *next = nullptr);
    void CopyFromImage2(VkCommandBuffer commandBuffer, const DeviceImage &source,
                        TKit::Span<const VkBufferImageCopy2KHR> copy, const void *next = nullptr);

#endif

    VKIT_NO_DISCARD Result<> CopyFromBuffer(CommandPool &pool, VkQueue queue, const DeviceBuffer &source,
                                            TKit::Span<const VkBufferCopy> copy);
    VKIT_NO_DISCARD Result<> CopyFromImage(CommandPool &pool, VkQueue queue, const DeviceImage &source,
                                           TKit::Span<const VkBufferImageCopy> copy);

    VKIT_NO_DISCARD Result<> UploadFromHost(CommandPool &pool, const VkQueue queue, const void *data,
                                            const VkBufferCopy &copy);

    const void *GetData() const
    {
        return m_Data;
    }
    void *GetData()
    {
        return m_Data;
    }

    VKIT_NO_DISCARD Result<> Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    VKIT_NO_DISCARD Result<> FlushAt(u32 index);

    VKIT_NO_DISCARD Result<> Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    VKIT_NO_DISCARD Result<> InvalidateAt(u32 index);

    template <typename Index> void BindAsIndexBuffer(VkCommandBuffer commandBuffer, VkDeviceSize offset = 0) const
    {
        static_assert(std::is_same_v<Index, u8> || std::is_same_v<Index, u16> || std::is_same_v<Index, u32>,
                      "[VULKIT][DEVICE-BUFFER] Index type must be u8, u16 or u32");
        if constexpr (std::is_same_v<Index, u8>)
            m_Device.Table->CmdBindIndexBuffer(commandBuffer, m_Buffer, offset, VK_INDEX_TYPE_UINT8_EXT);
        else if constexpr (std::is_same_v<Index, u16>)
            m_Device.Table->CmdBindIndexBuffer(commandBuffer, m_Buffer, offset, VK_INDEX_TYPE_UINT16);
        else if constexpr (std::is_same_v<Index, u32>)
            m_Device.Table->CmdBindIndexBuffer(commandBuffer, m_Buffer, offset, VK_INDEX_TYPE_UINT32);
    }
    void BindAsVertexBuffer(VkCommandBuffer commandBuffer, VkDeviceSize offset = 0) const;

    static void BindAsVertexBuffer(const ProxyDevice &device, VkCommandBuffer commandBuffer,
                                   TKit::Span<const VkBuffer> buffers, u32 firstBinding = 0,
                                   TKit::Span<const VkDeviceSize> offsets = {});

    VkDescriptorBufferInfo CreateDescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
    VkDescriptorBufferInfo CreateDescriptorInfoAt(u32 index) const;

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
