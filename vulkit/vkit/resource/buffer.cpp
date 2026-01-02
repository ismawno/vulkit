#include "vkit/core/pch.hpp"
#include "vkit/resource/buffer.hpp"
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

Buffer::Builder::Builder(const ProxyDevice &p_Device, const VmaAllocator p_Allocator, BufferFlags p_Flags)
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

    if ((p_Flags & BufferFlag_HostMapped) || (p_Flags & BufferFlag_HostRandomAccess))
        p_Flags |= BufferFlag_HostVisible;

    if (p_Flags & BufferFlag_Staging)
    {
        p_Flags |= BufferFlag_HostVisible;
        p_Flags |= BufferFlag_Source;
    }
    if (p_Flags & BufferFlag_DeviceLocal)
    {
        p_Flags |= BufferFlag_Destination;
        m_AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        m_AllocationInfo.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    if (p_Flags & BufferFlag_HostVisible)
    {
        m_AllocationInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        if (p_Flags & BufferFlag_HostRandomAccess)
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        else
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        if (p_Flags & BufferFlag_HostMapped)
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }
    if (p_Flags & BufferFlag_Source)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (p_Flags & BufferFlag_Destination)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (p_Flags & BufferFlag_Vertex)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (p_Flags & BufferFlag_Index)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (p_Flags & BufferFlag_Storage)
        m_BufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    m_Flags = p_Flags;
}

Result<Buffer> Buffer::Builder::Build() const
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCmdBindVertexBuffers, Result<Buffer>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCmdBindIndexBuffer, Result<Buffer>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCmdCopyBuffer, Result<Buffer>);

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
        return Result<Buffer>::Error(result, "Failed to create buffer");

    return Result<Buffer>::Ok(m_Device, buffer, info, data);
}

Buffer::Builder &Buffer::Builder::SetSize(const VkDeviceSize p_Size)
{
    m_InstanceCount = p_Size;
    m_InstanceSize = 1;
    return *this;
}
Buffer::Builder &Buffer::Builder::SetSize(const VkDeviceSize p_InstanceCount, const VkDeviceSize p_InstanceSize)
{
    m_InstanceCount = p_InstanceCount;
    m_InstanceSize = p_InstanceSize;
    return *this;
}
Buffer::Builder &Buffer::Builder::SetUsage(const VkBufferUsageFlags p_Flags)
{
    m_BufferInfo.usage = p_Flags;
    return *this;
}
Buffer::Builder &Buffer::Builder::SetSharingMode(const VkSharingMode p_Mode)
{
    m_BufferInfo.sharingMode = p_Mode;
    return *this;
}
Buffer::Builder &Buffer::Builder::SetAllocationCreateInfo(const VmaAllocationCreateInfo &p_Info)
{
    m_AllocationInfo = p_Info;
    return *this;
}
Buffer::Builder &Buffer::Builder::SetPerInstanceMinimumAlignment(const VkDeviceSize p_Alignment)
{
    m_PerInstanceMinimumAlignment = p_Alignment;
    return *this;
}
Buffer::Builder &Buffer::Builder::AddFamilyIndex(const u32 p_Index)
{
    m_FamilyIndices.Append(p_Index);
    return *this;
}

const VkBufferCreateInfo &Buffer::Builder::GetBufferInfo() const
{
    return m_BufferInfo;
}

void Buffer::Destroy()
{
    if (m_Buffer)
    {
        vmaDestroyBuffer(m_Info.Allocator, m_Buffer, m_Info.Allocation);
        m_Buffer = VK_NULL_HANDLE;
    }
}

Result<> Buffer::Map()
{
    TKIT_ASSERT(!m_Data, "[VULKIT] Buffer is already mapped");
    const VkResult result = vmaMapMemory(m_Info.Allocator, m_Info.Allocation, &m_Data);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to map buffer memory");

    return Result<>::Ok();
}

void Buffer::Unmap()
{
    TKIT_ASSERT(m_Data, "[VULKIT] Buffer is not mapped");
    vmaUnmapMemory(m_Info.Allocator, m_Info.Allocation);
    m_Data = nullptr;
}

void Buffer::Write(const void *p_Data, BufferCopy p_Info)
{
    if (p_Info.Size == VK_WHOLE_SIZE)
        p_Info.Size = m_Info.Size - p_Info.DstOffset;

    TKIT_ASSERT(m_Data, "[VULKIT] Cannot copy to unmapped buffer");
    TKIT_ASSERT(m_Info.Size >= p_Info.Size + p_Info.DstOffset, "[VULKIT] Buffer slice is smaller than the data size");

    std::byte *dst = static_cast<std::byte *>(m_Data) + p_Info.DstOffset;
    const std::byte *src = static_cast<const std::byte *>(p_Data) + p_Info.SrcOffset;
    TKit::Memory::ForwardCopy(dst, src, p_Info.Size);
}

void Buffer::WriteAt(const u32 p_Index, const void *p_Data)
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "[VULKIT] Index out of bounds");

    const VkDeviceSize size = m_Info.InstanceAlignedSize * p_Index;
    std::byte *data = static_cast<std::byte *>(m_Data) + size;
    TKit::Memory::ForwardCopy(data, p_Data, m_Info.InstanceSize);
}

void Buffer::CopyFromBuffer(const VkCommandBuffer p_CommandBuffer, const Buffer &p_Source, const BufferCopy &p_Info)
{
    VkBufferCopy copy{};
    copy.dstOffset = p_Info.DstOffset;
    copy.srcOffset = p_Info.SrcOffset;
    copy.size = p_Info.Size;

    TKIT_ASSERT(p_Source.GetInfo().Size >= copy.size, "[VULKIT] Specified size exceeds source buffer size");
    TKIT_ASSERT(m_Info.Size >= copy.size, "[VULKIT] Specified size exceeds destination buffer size");
    m_Device.Table->CmdCopyBuffer(p_CommandBuffer, p_Source, m_Buffer, 1, &copy);
}
void Buffer::CopyFromImage(const VkCommandBuffer p_CommandBuffer, const Image &p_Source, const BufferImageCopy &p_Info)
{
    const VkOffset3D &off = p_Info.ImageOffset;
    const VkExtent3D &ext = p_Info.Extent;
    const Image::Info &info = p_Source.GetInfo();
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
                "[VULKIT] 3D images cannot have multiple layers and array images cannot have depth > 1");
    TKIT_ASSERT(cext.width <= width - off.x, "[VULKIT] Specified width exceeds source image width");
    TKIT_ASSERT(cext.height <= height - off.y, "[VULKIT] Specified height exceeds source image height");
    TKIT_ASSERT(cext.depth <= depth - off.z, "[VULKIT] Specified depth exceeds source image depth");
    TKIT_ASSERT(m_Info.Size - p_Info.BufferOffset >=
                    p_Source.ComputeSize(p_Info.BufferRowLength ? p_Info.BufferRowLength : cext.width,
                                         p_Info.BufferImageHeight ? p_Info.BufferImageHeight : cext.height, 0,
                                         cext.depth) *
                        subr.layerCount,
                "[VULKIT] Buffer is not large enough to fit image");
    m_Device.Table->CmdCopyImageToBuffer(p_CommandBuffer, p_Source, p_Source.GetLayout(), m_Buffer, 1, &copy);
}

Result<> Buffer::CopyFromImage(CommandPool &p_Pool, VkQueue p_Queue, const Image &p_Source,
                               const BufferImageCopy &p_Info)
{
    const auto cres = p_Pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromImage(cmd, p_Source, p_Info);
    return p_Pool.EndSingleTimeCommands(cmd, p_Queue);
}

Result<> Buffer::CopyFromBuffer(CommandPool &p_Pool, VkQueue p_Queue, const Buffer &p_Source, const BufferCopy &p_Info)
{
    const auto cres = p_Pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromBuffer(cmd, p_Source, p_Info);
    return p_Pool.EndSingleTimeCommands(cmd, p_Queue);
}

Result<> Buffer::UploadFromHost(CommandPool &p_Pool, const VkQueue p_Queue, const void *p_Data,
                                const BufferCopy &p_Info)
{
    const VkDeviceSize size = p_Info.Size == VK_WHOLE_SIZE ? (m_Info.Size - p_Info.DstOffset) : p_Info.Size;

    auto bres =
        Buffer::Builder(m_Device, m_Info.Allocator, BufferFlag_HostMapped | BufferFlag_Staging).SetSize(size).Build();
    TKIT_RETURN_ON_ERROR(bres);

    Buffer &staging = bres.GetValue();
    staging.Write(p_Data, {.Size = size, .SrcOffset = p_Info.SrcOffset, .DstOffset = 0});
    staging.Flush();

    const auto cres =
        CopyFromBuffer(p_Pool, p_Queue, staging, {.Size = size, .SrcOffset = 0, .DstOffset = p_Info.DstOffset});
    staging.Destroy();
    return cres;
}

Result<> Buffer::Flush(const VkDeviceSize p_Size, const VkDeviceSize p_Offset)
{
    TKIT_ASSERT(m_Data, "[VULKIT] Cannot flush unmapped buffer");
    const VkResult result = vmaFlushAllocation(m_Info.Allocator, m_Info.Allocation, p_Offset, p_Size);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to flush buffer memory");
    return Result<>::Ok();
}
Result<> Buffer::FlushAt(const u32 p_Index)
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "[VULKIT] Index out of bounds");
    return Flush(m_Info.InstanceSize, m_Info.InstanceAlignedSize * p_Index);
}

Result<> Buffer::Invalidate(const VkDeviceSize p_Size, const VkDeviceSize p_Offset)
{
    TKIT_ASSERT(m_Data, "[VULKIT] Cannot invalidate unmapped buffer");
    const VkResult result = vmaInvalidateAllocation(m_Info.Allocator, m_Info.Allocation, p_Offset, p_Size);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to invalidate buffer memory");
    return Result<>::Ok();
}
Result<> Buffer::InvalidateAt(const u32 p_Index)
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "[VULKIT] Index out of bounds");
    return Invalidate(m_Info.InstanceSize, m_Info.InstanceAlignedSize * p_Index);
}

void Buffer::BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const VkDeviceSize p_Offset) const
{
    m_Device.Table->CmdBindVertexBuffers(p_CommandBuffer, 0, 1, &m_Buffer, &p_Offset);
}

void Buffer::BindAsVertexBuffer(const ProxyDevice &p_Device, const VkCommandBuffer p_CommandBuffer,
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

void Buffer::BindAsVertexBuffer(const VkCommandBuffer p_CommandBuffer, const VkBuffer p_Buffer,
                                const VkDeviceSize p_Offset) const
{
    m_Device.Table->CmdBindVertexBuffers(p_CommandBuffer, 0, 1, &p_Buffer, &p_Offset);
}

VkDescriptorBufferInfo Buffer::GetDescriptorInfo(const VkDeviceSize p_Size, const VkDeviceSize p_Offset) const
{
    VkDescriptorBufferInfo info{};
    info.buffer = m_Buffer;
    info.offset = p_Offset;
    info.range = p_Size;
    return info;
}
VkDescriptorBufferInfo Buffer::GetDescriptorInfoAt(const u32 p_Index) const
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "[VULKIT] Index out of bounds");
    return GetDescriptorInfo(m_Info.InstanceSize, m_Info.InstanceAlignedSize * p_Index);
}

} // namespace VKit
