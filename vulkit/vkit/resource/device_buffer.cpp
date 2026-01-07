#include "vkit/core/pch.hpp"
#include "vkit/resource/device_buffer.hpp"
#include "vkit/execution/command_pool.hpp"
#include "vkit/resource/image.hpp"
#include "vkit/resource/host_buffer.hpp"
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
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCmdBindVertexBuffers, Result<DeviceBuffer>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCmdBindIndexBuffer, Result<DeviceBuffer>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCmdCopyBuffer, Result<DeviceBuffer>);

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

void DeviceBuffer::Write(const void *p_Data, BufferCopy p_Info)
{
    if (p_Info.Size == VK_WHOLE_SIZE)
        p_Info.Size = m_Info.Size - p_Info.DstOffset;

    TKIT_ASSERT(m_Data, "[VULKIT][DEVICE-BUFFER] Cannot copy to unmapped buffer");
    TKIT_ASSERT(m_Info.Size >= p_Info.Size + p_Info.DstOffset,
                "[VULKIT][DEVICE-BUFFER] Buffer slice ({}) is smaller than the data size ({})", m_Info.Size,
                p_Info.Size + p_Info.DstOffset);

    std::byte *dst = static_cast<std::byte *>(m_Data) + p_Info.DstOffset;
    const std::byte *src = static_cast<const std::byte *>(p_Data) + p_Info.SrcOffset;
    TKit::Memory::ForwardCopy(dst, src, p_Info.Size);
}
void DeviceBuffer::Write(const HostBuffer &p_Data, const BufferCopy &p_Info)
{
    const VkDeviceSize size = p_Info.Size == VK_WHOLE_SIZE ? (p_Data.GetSize() - p_Info.SrcOffset) : (p_Info.Size);
    Write(p_Data.GetData(), {.Size = size, .SrcOffset = p_Info.SrcOffset, .DstOffset = p_Info.DstOffset});
}

void DeviceBuffer::WriteAt(const u32 p_Index, const void *p_Data)
{
    TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Info.InstanceCount, "[VULKIT][DEVICE-BUFFER] ");

    const VkDeviceSize size = m_Info.InstanceAlignedSize * p_Index;
    std::byte *data = static_cast<std::byte *>(m_Data) + size;
    TKit::Memory::ForwardCopy(data, p_Data, m_Info.InstanceSize);
}

void DeviceBuffer::CopyFromBuffer(const VkCommandBuffer p_CommandBuffer, const DeviceBuffer &p_Source,
                                  const BufferCopy &p_Info)
{
    VkBufferCopy copy{};
    copy.dstOffset = p_Info.DstOffset;
    copy.srcOffset = p_Info.SrcOffset;
    copy.size = p_Info.Size;

    TKIT_ASSERT(p_Source.GetInfo().Size - copy.srcOffset >= copy.size,
                "[VULKIT][DEVICE-BUFFER] Specified size ({}) exceeds source buffer size ({})", copy.size,
                p_Source.GetInfo().Size - copy.srcOffset);

    TKIT_ASSERT(m_Info.Size - copy.dstOffset >= copy.size,
                "[VULKIT][DEVICE-BUFFER] Specified size ({}) exceeds destination buffer size ({})", copy.size,
                m_Info.Size - copy.dstOffset);

    m_Device.Table->CmdCopyBuffer(p_CommandBuffer, p_Source, m_Buffer, 1, &copy);
}
void DeviceBuffer::CopyFromImage(const VkCommandBuffer p_CommandBuffer, const DeviceImage &p_Source,
                                 const BufferImageCopy &p_Info)
{
    const VkOffset3D &off = p_Info.ImageOffset;
    const VkExtent3D &ext = p_Info.Extent;
    const DeviceImage::Info &info = p_Source.GetInfo();
    const VkImageSubresourceLayers &subr = p_Info.Subresource;

    const u32 width = Math::Max(1u, info.Width >> subr.mipLevel);
    const u32 height = Math::Max(1u, info.Height >> subr.mipLevel);
    const u32 depth = Math::Max(1u, info.Depth >> subr.mipLevel);

    VkBufferImageCopy copy;
    copy.bufferImageHeight = p_Info.BufferImageHeight;
    copy.bufferOffset = p_Info.BufferOffset;
    copy.imageOffset = off;
    copy.bufferRowLength = p_Info.BufferRowLength;
    copy.imageSubresource = subr;
    if (copy.imageSubresource.aspectMask == VK_IMAGE_ASPECT_NONE)
        copy.imageSubresource.aspectMask = Detail::DeduceAspectMask(p_Source.GetInfo().Flags);

    VkExtent3D &cext = copy.imageExtent;
    cext.width = ext.width == TKIT_U32_MAX ? (width - off.x) : ext.width;
    cext.height = ext.height == TKIT_U32_MAX ? (height - off.y) : ext.height;
    cext.depth = ext.depth == TKIT_U32_MAX ? (depth - off.z) : ext.depth;

    // i know this is so futile, validation layers would already catch this but well...
    TKIT_ASSERT(subr.layerCount == 1 || info.Depth == 1,
                "[VULKIT][DEVICE-BUFFER] 3D images cannot have multiple layers and array images cannot have a depth "
                "greather than 1. "
                "Layers: {}, depth: {}",
                subr.layerCount, info.Depth);

    TKIT_ASSERT(cext.width <= width - off.x,
                "[VULKIT][DEVICE-BUFFER] Specified width ({}) exceeds source image width ({})", width - off.x,
                cext.width);
    TKIT_ASSERT(cext.height <= height - off.x,
                "[VULKIT][DEVICE-BUFFER] Specified height ({}) exceeds source image height ({})", height - off.x,
                cext.width);
    TKIT_ASSERT(cext.depth <= depth - off.x,
                "[VULKIT][DEVICE-BUFFER] Specified depth ({}) exceeds source image depth ({})", depth - off.x,
                cext.width);
    TKIT_ASSERT(subr.layerCount == 1 || info.Depth == 1,
                "[VULKIT][DEVICE-BUFFER] 3D images cannot have multiple layers and array images cannot have depth > 1");
#ifdef TKIT_ENABLE_ASSERTS
    const VkDeviceSize bsize = m_Info.Size - p_Info.BufferOffset;
    const VkDeviceSize isize =
        p_Source.ComputeSize(p_Info.BufferRowLength ? p_Info.BufferRowLength : cext.width,
                             p_Info.BufferImageHeight ? p_Info.BufferImageHeight : cext.height, 0, cext.depth) *
        subr.layerCount;

    TKIT_ASSERT(bsize >= isize, "[VULKIT][DEVICE-BUFFER] Buffer of size {} is not large enough to fit image of size {}",
                bsize, isize);
#endif
    m_Device.Table->CmdCopyImageToBuffer(p_CommandBuffer, p_Source, p_Source.GetLayout(), m_Buffer, 1, &copy);
}

Result<> DeviceBuffer::CopyFromImage(CommandPool &p_Pool, VkQueue p_Queue, const DeviceImage &p_Source,
                                     const BufferImageCopy &p_Info)
{
    const auto cres = p_Pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromImage(cmd, p_Source, p_Info);
    return p_Pool.EndSingleTimeCommands(cmd, p_Queue);
}

Result<> DeviceBuffer::CopyFromBuffer(CommandPool &p_Pool, VkQueue p_Queue, const DeviceBuffer &p_Source,
                                      const BufferCopy &p_Info)
{
    const auto cres = p_Pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromBuffer(cmd, p_Source, p_Info);
    return p_Pool.EndSingleTimeCommands(cmd, p_Queue);
}

Result<> DeviceBuffer::UploadFromHost(CommandPool &p_Pool, const VkQueue p_Queue, const void *p_Data,
                                      const BufferCopy &p_Info)
{
    const VkDeviceSize size = p_Info.Size == VK_WHOLE_SIZE ? (m_Info.Size - p_Info.DstOffset) : p_Info.Size;

    auto bres =
        DeviceBuffer::Builder(m_Device, m_Info.Allocator, DeviceBufferFlag_HostMapped | DeviceBufferFlag_Staging)
            .SetSize(size)
            .Build();
    TKIT_RETURN_ON_ERROR(bres);

    DeviceBuffer &staging = bres.GetValue();
    staging.Write(p_Data, {.Size = size, .SrcOffset = p_Info.SrcOffset, .DstOffset = 0});
    const auto result = staging.Flush();
    TKIT_RETURN_ON_ERROR(result);

    const auto cres =
        CopyFromBuffer(p_Pool, p_Queue, staging, {.Size = size, .SrcOffset = 0, .DstOffset = p_Info.DstOffset});
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
    if (!p_Offsets.IsEmpty())
        p_Device.Table->CmdBindVertexBuffers(p_CommandBuffer, p_FirstBinding, p_Buffers.GetSize(), p_Buffers.GetData(),
                                             p_Offsets.GetData());
    else
        p_Device.Table->CmdBindVertexBuffers(p_CommandBuffer, p_FirstBinding, p_Buffers.GetSize(), p_Buffers.GetData(),
                                             nullptr);
}

void DeviceBuffer::BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const VkBuffer p_Buffer,
                                      const VkDeviceSize p_Offset) const
{
    m_Device.Table->CmdBindVertexBuffers(p_CommandBuffer, 0, 1, &p_Buffer, &p_Offset);
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
