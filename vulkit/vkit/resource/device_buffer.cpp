#include "vkit/core/pch.hpp"
#include "vkit/resource/device_buffer.hpp"
#include "vkit/execution/command_pool.hpp"
#include "vkit/resource/image.hpp"
#include "tkit/math/math.hpp"

namespace VKit
{
namespace Math = TKit::Math;
static VkDeviceSize alignedSize(const VkDeviceSize size, const VkDeviceSize alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

DeviceBuffer::Builder::Builder(const ProxyDevice &device, const VmaAllocator allocator, DeviceBufferFlags flags)
    : m_Device(device), m_Allocator(allocator)
{
    m_AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    m_AllocationInfo.requiredFlags = 0;
    m_AllocationInfo.preferredFlags = 0;
    m_AllocationInfo.flags = 0;

    m_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    m_BufferInfo.usage = 0;
    m_BufferInfo.flags = 0;
    m_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if ((flags & DeviceBufferFlag_HostMapped) || (flags & DeviceBufferFlag_HostRandomAccess))
        flags |= DeviceBufferFlag_HostVisible;

    if (flags & DeviceBufferFlag_Staging)
    {
        flags |= DeviceBufferFlag_HostVisible;
        flags |= DeviceBufferFlag_Source;
    }
    if (flags & DeviceBufferFlag_DeviceLocal)
    {
        flags |= DeviceBufferFlag_Destination;
        m_AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        m_AllocationInfo.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    if (flags & DeviceBufferFlag_HostVisible)
    {
        m_AllocationInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        if (flags & DeviceBufferFlag_HostRandomAccess)
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        else
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        if (flags & DeviceBufferFlag_HostMapped)
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }
    if (flags & DeviceBufferFlag_Source)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (flags & DeviceBufferFlag_Destination)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (flags & DeviceBufferFlag_Vertex)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (flags & DeviceBufferFlag_Index)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (flags & DeviceBufferFlag_Storage)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    m_Flags = flags;
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

DeviceBuffer::Builder &DeviceBuffer::Builder::SetSize(const VkDeviceSize size)
{
    m_InstanceCount = size;
    m_InstanceSize = 1;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::SetSize(const VkDeviceSize instanceCount, const VkDeviceSize instanceSize)
{
    m_InstanceCount = instanceCount;
    m_InstanceSize = instanceSize;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::SetUsage(const VkBufferUsageFlags flags)
{
    m_BufferInfo.usage = flags;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::SetSharingMode(const VkSharingMode mode)
{
    m_BufferInfo.sharingMode = mode;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::SetAllocationCreateInfo(const VmaAllocationCreateInfo &info)
{
    m_AllocationInfo = info;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::SetPerInstanceMinimumAlignment(const VkDeviceSize alignment)
{
    m_PerInstanceMinimumAlignment = alignment;
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::AddFamilyIndex(const u32 index)
{
    m_FamilyIndices.Append(index);
    return *this;
}
DeviceBuffer::Builder &DeviceBuffer::Builder::SetNext(const void *next)
{
    m_BufferInfo.pNext = next;
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

void DeviceBuffer::Write(const void *data, const VkBufferCopy &copy)
{
    TKIT_ASSERT(m_Data, "[VULKIT][DEVICE-BUFFER] Cannot copy to unmapped buffer");
    TKIT_ASSERT(m_Info.Size >= copy.size + copy.dstOffset,
                "[VULKIT][DEVICE-BUFFER] Copy size ({}) must be smaller or equal than the buffer size ({}) minus "
                "destination offset ({})",
                copy.size, m_Info.Size, copy.dstOffset);

    std::byte *dst = static_cast<std::byte *>(m_Data) + copy.dstOffset;
    const std::byte *src = static_cast<const std::byte *>(data) + copy.srcOffset;
    TKit::ForwardCopy(dst, src, copy.size);
}

void DeviceBuffer::WriteAt(const u32 index, const void *pdata)
{
    TKIT_CHECK_OUT_OF_BOUNDS(index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");

    const VkDeviceSize size = m_Info.InstanceAlignedSize * index;
    std::byte *data = static_cast<std::byte *>(m_Data) + size;
    TKit::ForwardCopy(data, pdata, m_Info.InstanceSize);
}

void DeviceBuffer::CopyFromBuffer(const VkCommandBuffer commandBuffer, const DeviceBuffer &source,
                                  const TKit::Span<const VkBufferCopy> copy)
{
    m_Device.Table->CmdCopyBuffer(commandBuffer, source, m_Buffer, copy.GetSize(), copy.GetData());
}

void DeviceBuffer::CopyFromImage(VkCommandBuffer commandBuffer, const DeviceImage &source,
                                 const TKit::Span<const VkBufferImageCopy> copy)
{
    m_Device.Table->CmdCopyImageToBuffer(commandBuffer, source, source.GetLayout(), m_Buffer, copy.GetSize(),
                                         copy.GetData());
}

#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_copy_commands2)
void DeviceBuffer::CopyFromBuffer2(const VkCommandBuffer commandBuffer, const DeviceBuffer &source,
                                   const TKit::Span<const VkBufferCopy2KHR> copy, const void *next)
{
    VkCopyBufferInfo2KHR info{};
    info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2_KHR;
    info.pNext = next;
    info.srcBuffer = source;
    info.dstBuffer = m_Buffer;
    info.pRegions = copy.GetData();
    info.regionCount = copy.GetSize();

    m_Device.Table->CmdCopyBuffer2KHR(commandBuffer, &info);
}
void DeviceBuffer::CopyFromImage2(VkCommandBuffer commandBuffer, const DeviceImage &source,
                                  TKit::Span<const VkBufferImageCopy2KHR> copy, const void *next)
{
    VkCopyImageToBufferInfo2KHR info{};
    info.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2_KHR;
    info.pNext = next;
    info.srcImage = source;
    info.dstBuffer = m_Buffer;
    info.srcImageLayout = source.GetLayout();
    info.pRegions = copy.GetData();
    info.regionCount = copy.GetSize();
    m_Device.Table->CmdCopyImageToBuffer2KHR(commandBuffer, &info);
}
#endif

Result<> DeviceBuffer::CopyFromBuffer(CommandPool &pool, VkQueue queue, const DeviceBuffer &source,
                                      const TKit::Span<const VkBufferCopy> copy)
{
    const auto cres = pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromBuffer(cmd, source, copy);
    return pool.EndSingleTimeCommands(cmd, queue);
}

Result<> DeviceBuffer::CopyFromImage(CommandPool &pool, VkQueue queue, const DeviceImage &source,
                                     const TKit::Span<const VkBufferImageCopy> copy)
{
    const auto cres = pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromImage(cmd, source, copy);
    return pool.EndSingleTimeCommands(cmd, queue);
}

Result<> DeviceBuffer::UploadFromHost(CommandPool &pool, const VkQueue queue, const void *data,
                                      const VkBufferCopy &pcopy)
{
    auto bres =
        DeviceBuffer::Builder(m_Device, m_Info.Allocator, DeviceBufferFlag_HostMapped | DeviceBufferFlag_Staging)
            .SetSize(pcopy.size)
            .Build();
    TKIT_RETURN_ON_ERROR(bres);

    DeviceBuffer &staging = bres.GetValue();
    staging.Write(data, {.srcOffset = pcopy.srcOffset, .dstOffset = 0, .size = pcopy.size});
    const auto result = staging.Flush();
    TKIT_RETURN_ON_ERROR(result, staging.Destroy());

    const VkBufferCopy copy{.srcOffset = 0, .dstOffset = pcopy.dstOffset, .size = pcopy.size};
    const auto cres = CopyFromBuffer(pool, queue, staging, copy);
    staging.Destroy();
    return cres;
}

Result<> DeviceBuffer::Flush(const VkDeviceSize size, const VkDeviceSize offset)
{
    TKIT_ASSERT(m_Data, "[VULKIT][DEVICE-BUFFER] Cannot flush unmapped buffer");
    const VkResult result = vmaFlushAllocation(m_Info.Allocator, m_Info.Allocation, offset, size);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);
    return Result<>::Ok();
}
Result<> DeviceBuffer::FlushAt(const u32 index)
{
    TKIT_CHECK_OUT_OF_BOUNDS(index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");
    return Flush(m_Info.InstanceSize, m_Info.InstanceAlignedSize * index);
}

Result<> DeviceBuffer::Invalidate(const VkDeviceSize size, const VkDeviceSize offset)
{
    TKIT_ASSERT(m_Data, "[VULKIT][DEVICE-BUFFER] Cannot invalidate unmapped buffer");
    const VkResult result = vmaInvalidateAllocation(m_Info.Allocator, m_Info.Allocation, offset, size);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);
    return Result<>::Ok();
}
Result<> DeviceBuffer::InvalidateAt(const u32 index)
{
    TKIT_CHECK_OUT_OF_BOUNDS(index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");
    return Invalidate(m_Info.InstanceSize, m_Info.InstanceAlignedSize * index);
}

void DeviceBuffer::BindAsVertexBuffer(const VkCommandBuffer commandBuffer, const VkDeviceSize offset) const
{
    m_Device.Table->CmdBindVertexBuffers(commandBuffer, 0, 1, &m_Buffer, &offset);
}

void DeviceBuffer::BindAsVertexBuffer(const ProxyDevice &device, const VkCommandBuffer commandBuffer,
                                      const TKit::Span<const VkBuffer> buffers, const u32 firstBinding,
                                      const TKit::Span<const VkDeviceSize> offsets)
{
    device.Table->CmdBindVertexBuffers(commandBuffer, firstBinding, buffers.GetSize(), buffers.GetData(),
                                       offsets.GetData());
}

VkDescriptorBufferInfo DeviceBuffer::CreateDescriptorInfo(const VkDeviceSize size, const VkDeviceSize offset) const
{
    VkDescriptorBufferInfo info{};
    info.buffer = m_Buffer;
    info.offset = offset;
    info.range = size;
    return info;
}
VkDescriptorBufferInfo DeviceBuffer::CreateDescriptorInfoAt(const u32 index) const
{
    TKIT_CHECK_OUT_OF_BOUNDS(index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");
    return CreateDescriptorInfo(m_Info.InstanceSize, m_Info.InstanceAlignedSize * index);
}

} // namespace VKit
