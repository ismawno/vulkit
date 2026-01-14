#include "vkit/core/pch.hpp"
#include "vkit/resource/device_buffer.hpp"
#include "vkit/execution/command_pool.hpp"
#include "vkit/resource/image.hpp"
#include "tkit/math/math.hpp"

namespace VKit
{
namespace Math = TKit::Math;
static VkDeviceSize alignedSize(const VkDeviceSize p_Size, const VkDeviceSize p_Alignment)
{
    return (p_Size + p_Alignment - 1) & ~(p_Alignment - 1);
}

DeviceBuffer::Builder::Builder(const ProxyDevice &p_Device, const VmaAllocator p_Allocator, DeviceBufferFlags p_Flags)
    : m_Device(p_Device), m_Allocator(p_Allocator)
{
    m_AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    m_AllocationInfo.requiredFlags = 0;
    m_AllocationInfo.preferredFlags = 0;
    m_AllocationInfo.flags = 0;

    m_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    m_BufferInfo.usage = 0;
    m_BufferInfo.flags = 0;
    m_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if ((p_Flags & DeviceBufferFlag_HostMapped) || (p_Flags & DeviceBufferFlag_HostRandomAccess))
        p_Flags |= DeviceBufferFlag_HostVisible;

    if (p_Flags & DeviceBufferFlag_Staging)
    {
        p_Flags |= DeviceBufferFlag_HostVisible;
        p_Flags |= DeviceBufferFlag_Source;
    }
    if (p_Flags & DeviceBufferFlag_DeviceLocal)
    {
        p_Flags |= DeviceBufferFlag_Destination;
        m_AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        m_AllocationInfo.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    if (p_Flags & DeviceBufferFlag_HostVisible)
    {
        m_AllocationInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        if (p_Flags & DeviceBufferFlag_HostRandomAccess)
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        else
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        if (p_Flags & DeviceBufferFlag_HostMapped)
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }
    if (p_Flags & DeviceBufferFlag_Source)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (p_Flags & DeviceBufferFlag_Destination)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (p_Flags & DeviceBufferFlag_Vertex)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (p_Flags & DeviceBufferFlag_Index)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (p_Flags & DeviceBufferFlag_Storage)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    m_Flags = p_Flags;
}

Result<DeviceBuffer> DeviceBuffer::Builder::Build() const
{
    Info info;
    info.Allocator = m_Allocator;
    info.InstanceSize = m_InstanceSize;
    info.InstanceCount = m_InstanceCount;
    info.InstanceAlignedSize = alignedSize(m_InstanceSize, m_PerInstanceMinimumAlignment);
    info.Size = info.InstanceAlignedSize * m_InstanceCount;
    info.Flags = m_Flags;

    VkBufferCreateInfo bufferInfo = m_BufferInfo;
    bufferInfo.size = info.Size;

    if (!m_FamilyIndices.IsEmpty())
    {
        bufferInfo.pQueueFamilyIndices = m_FamilyIndices.GetData();
        bufferInfo.queueFamilyIndexCount = m_FamilyIndices.GetSize();
    }
    else
    {
        bufferInfo.pQueueFamilyIndices = nullptr;
        bufferInfo.queueFamilyIndexCount = 0;
    }

    VkBuffer buffer;
    VmaAllocationInfo allocationInfo;
    const VkResult result =
        vmaCreateBuffer(m_Allocator, &bufferInfo, &m_AllocationInfo, &buffer, &info.Allocation, &allocationInfo);

    void *data = nullptr;
    if (m_AllocationInfo.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
        data = allocationInfo.pMappedData;

    if (result != VK_SUCCESS)
        return Result<DeviceBuffer>::Error(result);

    return Result<DeviceBuffer>::Ok(m_Device, buffer, info, data);
}

DeviceBuffer::Builder &DeviceBuffer::Builder::SetSize(const VkDeviceSize p_Size)
{
    m_InstanceCount = p_Size;
    m_InstanceSize = 1;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::SetSize(const VkDeviceSize p_InstanceCount,
                                                      const VkDeviceSize p_InstanceSize)
{
    m_InstanceCount = p_InstanceCount;
    m_InstanceSize = p_InstanceSize;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::SetUsage(const VkBufferUsageFlags p_Flags)
{
    m_BufferInfo.usage = p_Flags;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::SetSharingMode(const VkSharingMode p_Mode)
{
    m_BufferInfo.sharingMode = p_Mode;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::SetAllocationCreateInfo(const VmaAllocationCreateInfo &p_Info)
{
    m_AllocationInfo = p_Info;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::SetPerInstanceMinimumAlignment(const VkDeviceSize p_Alignment)
{
    m_PerInstanceMinimumAlignment = p_Alignment;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::AddFamilyIndex(const u32 p_Index)
{
    m_FamilyIndices.Append(p_Index);
    return *this;
}

const VkBufferCreateInfo &DeviceBuffer::Builder::GetBufferInfo() const
{
    return m_BufferInfo;
}

void DeviceBuffer::Destroy()
{
    if (m_Buffer)
    {
        vmaDestroyBuffer(m_Info.Allocator, m_Buffer, m_Info.Allocation);
        m_Buffer = VK_NULL_HANDLE;
    }
}

Result<> DeviceBuffer::Map()
{
    TKIT_ASSERT(!m_Data, "[VULKIT][DEVICE-BUFFER] Buffer is already mapped");
    const VkResult result = vmaMapMemory(m_Info.Allocator, m_Info.Allocation, &m_Data);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);

    return Result<>::Ok();
}

void DeviceBuffer::Unmap()
{
    TKIT_ASSERT(m_Data, "[VULKIT][DEVICE-BUFFER] Buffer is not mapped");
    vmaUnmapMemory(m_Info.Allocator, m_Info.Allocation);
    m_Data = nullptr;
}

void DeviceBuffer::Write(const void *p_Data, const VkBufferCopy &p_Copy)
{
    TKIT_ASSERT(m_Data, "[VULKIT][DEVICE-BUFFER] Cannot copy to unmapped buffer");
    TKIT_ASSERT(m_Info.Size >= p_Copy.size + p_Copy.dstOffset,
                "[VULKIT][DEVICE-BUFFER] Buffer slice ({}) is smaller than the data size ({})", m_Info.Size,
                p_Copy.size + p_Copy.dstOffset);

    std::byte *dst = static_cast<std::byte *>(m_Data) + p_Copy.dstOffset;
    const std::byte *src = static_cast<const std::byte *>(p_Data) + p_Copy.srcOffset;
    TKit::Memory::ForwardCopy(dst, src, p_Copy.size);
}

void DeviceBuffer::WriteAt(const u32 p_Index, const void *p_Data)
{
    TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");

    const VkDeviceSize size = m_Info.InstanceAlignedSize * p_Index;
    std::byte *data = static_cast<std::byte *>(m_Data) + size;
    TKit::Memory::ForwardCopy(data, p_Data, m_Info.InstanceSize);
}

void DeviceBuffer::CopyFromBuffer(const VkCommandBuffer p_CommandBuffer, const DeviceBuffer &p_Source,
                                  const TKit::Span<const VkBufferCopy> p_Copy)
{
    m_Device.Table->CmdCopyBuffer(p_CommandBuffer, p_Source, m_Buffer, p_Copy.GetSize(), p_Copy.GetData());
}

void DeviceBuffer::CopyFromImage(VkCommandBuffer p_CommandBuffer, const DeviceImage &p_Source,
                                 const TKit::Span<const VkBufferImageCopy> p_Copy)
{
    m_Device.Table->CmdCopyImageToBuffer(p_CommandBuffer, p_Source, p_Source.GetLayout(), m_Buffer, p_Copy.GetSize(),
                                         p_Copy.GetData());
}

#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_copy_commands2)
void DeviceBuffer::CopyFromBuffer2(const VkCommandBuffer p_CommandBuffer, const DeviceBuffer &p_Source,
                                   const TKit::Span<const VkBufferCopy2KHR> p_Copy, const void *p_Next)
{
    VkCopyBufferInfo2KHR info{};
    info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2_KHR;
    info.pNext = p_Next;
    info.srcBuffer = p_Source;
    info.dstBuffer = m_Buffer;
    info.pRegions = p_Copy.GetData();
    info.regionCount = p_Copy.GetSize();

    m_Device.Table->CmdCopyBuffer2KHR(p_CommandBuffer, &info);
}
void DeviceBuffer::CopyFromImage2(VkCommandBuffer p_CommandBuffer, const DeviceImage &p_Source,
                                  TKit::Span<const VkBufferImageCopy2KHR> p_Copy, const void *p_Next)
{
    VkCopyImageToBufferInfo2KHR info{};
    info.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2_KHR;
    info.pNext = p_Next;
    info.srcImage = p_Source;
    info.dstBuffer = m_Buffer;
    info.srcImageLayout = p_Source.GetLayout();
    info.pRegions = p_Copy.GetData();
    info.regionCount = p_Copy.GetSize();
    m_Device.Table->CmdCopyImageToBuffer2KHR(p_CommandBuffer, &info);
}
#endif

Result<> DeviceBuffer::CopyFromBuffer(CommandPool &p_Pool, VkQueue p_Queue, const DeviceBuffer &p_Source,
                                      const TKit::Span<const VkBufferCopy> p_Copy)
{
    const auto cres = p_Pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromBuffer(cmd, p_Source, p_Copy);
    return p_Pool.EndSingleTimeCommands(cmd, p_Queue);
}

Result<> DeviceBuffer::CopyFromImage(CommandPool &p_Pool, VkQueue p_Queue, const DeviceImage &p_Source,
                                     const TKit::Span<const VkBufferImageCopy> p_Copy)
{
    const auto cres = p_Pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromImage(cmd, p_Source, p_Copy);
    return p_Pool.EndSingleTimeCommands(cmd, p_Queue);
}

Result<> DeviceBuffer::UploadFromHost(CommandPool &p_Pool, const VkQueue p_Queue, const void *p_Data,
                                      const VkBufferCopy &p_Copy)
{
    auto bres =
        DeviceBuffer::Builder(m_Device, m_Info.Allocator, DeviceBufferFlag_HostMapped | DeviceBufferFlag_Staging)
            .SetSize(p_Copy.size)
            .Build();
    TKIT_RETURN_ON_ERROR(bres);

    DeviceBuffer &staging = bres.GetValue();
    staging.Write(p_Data, {.srcOffset = p_Copy.srcOffset, .dstOffset = 0, .size = p_Copy.size});
    const auto result = staging.Flush();
    TKIT_RETURN_ON_ERROR(result);

    const VkBufferCopy copy{.srcOffset = 0, .dstOffset = p_Copy.dstOffset, .size = p_Copy.size};
    const auto cres = CopyFromBuffer(p_Pool, p_Queue, staging, copy);
    staging.Destroy();
    return cres;
}

Result<> DeviceBuffer::Flush(const VkDeviceSize p_Size, const VkDeviceSize p_Offset)
{
    TKIT_ASSERT(m_Data, "[VULKIT][DEVICE-BUFFER] Cannot flush unmapped buffer");
    const VkResult result = vmaFlushAllocation(m_Info.Allocator, m_Info.Allocation, p_Offset, p_Size);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);
    return Result<>::Ok();
}
Result<> DeviceBuffer::FlushAt(const u32 p_Index)
{
    TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");
    return Flush(m_Info.InstanceSize, m_Info.InstanceAlignedSize * p_Index);
}

Result<> DeviceBuffer::Invalidate(const VkDeviceSize p_Size, const VkDeviceSize p_Offset)
{
    TKIT_ASSERT(m_Data, "[VULKIT][DEVICE-BUFFER] Cannot invalidate unmapped buffer");
    const VkResult result = vmaInvalidateAllocation(m_Info.Allocator, m_Info.Allocation, p_Offset, p_Size);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);
    return Result<>::Ok();
}
Result<> DeviceBuffer::InvalidateAt(const u32 p_Index)
{
    TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");
    return Invalidate(m_Info.InstanceSize, m_Info.InstanceAlignedSize * p_Index);
}

void DeviceBuffer::BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset) const
{
    m_Device.Table->CmdBindVertexBuffers(p_CommandBuffer, 0, 1, &m_Buffer, &p_Offset);
}

void DeviceBuffer::BindAsVertexBuffer(const ProxyDevice &p_Device, const VkCommandBuffer p_CommandBuffer,
                                      const TKit::Span<const VkBuffer> p_Buffers, const u32 p_FirstBinding,
                                      const TKit::Span<const VkDeviceSize> p_Offsets)
{
    p_Device.Table->CmdBindVertexBuffers(p_CommandBuffer, p_FirstBinding, p_Buffers.GetSize(), p_Buffers.GetData(),
                                         p_Offsets.GetData());
}

VkDescriptorBufferInfo DeviceBuffer::CreateDescriptorInfo(const VkDeviceSize p_Size, const VkDeviceSize p_Offset) const
{
    VkDescriptorBufferInfo info{};
    info.buffer = m_Buffer;
    info.offset = p_Offset;
    info.range = p_Size;
    return info;
}
VkDescriptorBufferInfo DeviceBuffer::CreateDescriptorInfoAt(const u32 p_Index) const
{
    TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");
    return CreateDescriptorInfo(m_Info.InstanceSize, m_Info.InstanceAlignedSize * p_Index);
}

} // namespace VKit
