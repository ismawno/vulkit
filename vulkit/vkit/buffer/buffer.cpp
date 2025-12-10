#include "vkit/core/pch.hpp"
#include "vkit/buffer/buffer.hpp"
#include "vkit/rendering/command_pool.hpp"

namespace VKit
{
static VkDeviceSize alignedSize(const VkDeviceSize p_Size, const VkDeviceSize p_Alignment)
{
    return (p_Size + p_Alignment - 1) & ~(p_Alignment - 1);
}

Buffer::Builder::Builder(const LogicalDevice::Proxy &p_Device, const VmaAllocator p_Allocator, Flags p_Flags)
    : m_Device(p_Device), m_Allocator(p_Allocator)
{
#ifdef TKIT_ENABLE_ASSERTS
    const auto exclusive = [p_Flags](const Flags p_Flags1, const Flags p_Flags2, const char *p_Message) {
        TKIT_ASSERT(!((p_Flags & p_Flags1) && (p_Flags & p_Flags2)), "{}", p_Message);
    };

    exclusive(Flag_DeviceLocal, Flag_HostVisible, "[ONYX] A device local buffer cannot be host visible");
    exclusive(Flag_DeviceLocal, Flag_Mapped, "[ONYX] A device local buffer cannot be mapped");
    exclusive(Flag_DeviceLocal, Flag_RandomAccess, "[ONYX] A device local buffer cannot be randomly accessed");
    exclusive(Flag_VertexBuffer, Flag_IndexBuffer, "[ONYX] A vertex buffer cannot be an index buffer");
    exclusive(Flag_VertexBuffer, Flag_StagingBuffer, "[ONYX] A vertex buffer cannot be a staging buffer");
    exclusive(Flag_IndexBuffer, Flag_StagingBuffer, "[ONYX] An index buffer cannot be a staging buffer");
#endif
    m_AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    m_AllocationInfo.requiredFlags = 0;
    m_AllocationInfo.preferredFlags = 0;
    m_AllocationInfo.flags = 0;
    if ((p_Flags & Flag_Mapped) || (p_Flags & Flag_RandomAccess))
        p_Flags |= Flag_HostVisible;

    if (p_Flags & Flag_StagingBuffer)
    {
        p_Flags |= Flag_HostVisible;
        p_Flags |= Flag_Source;
    }
    if (p_Flags & Flag_DeviceLocal)
    {
        p_Flags |= Flag_Destination;
        m_AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        m_AllocationInfo.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    if (p_Flags & Flag_HostVisible)
    {
        m_AllocationInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        if (p_Flags & Flag_RandomAccess)
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        else
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        if (p_Flags & Flag_Mapped)
            m_AllocationInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }
    if (p_Flags & Flag_VertexBuffer)
        m_Usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    else if (p_Flags & Flag_IndexBuffer)
        m_Usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
}

Result<Buffer> Buffer::Builder::Build() const
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCmdBindVertexBuffers, Result<Buffer>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCmdBindIndexBuffer, Result<Buffer>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCmdCopyBuffer, Result<Buffer>);

    Info info{};
    info.Allocator = m_Allocator;
    info.InstanceSize = m_InstanceSize;
    info.InstanceCount = m_InstanceCount;
    info.InstanceAlignedSize = alignedSize(m_InstanceSize, m_PerInstanceMinimumAlignment);
    info.Size = info.InstanceAlignedSize * m_InstanceCount;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = info.Size;
    bufferInfo.usage = m_Usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

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
    m_Usage = p_Flags;
    return *this;
}
Buffer::Builder &Buffer::Builder::SetAllocationCreateInfo(const VmaAllocationCreateInfo &p_Info)
{
    m_AllocationInfo = p_Info;
    return *this;
}

void Buffer::Destroy()
{
    vmaDestroyBuffer(m_Info.Allocator, m_Buffer, m_Info.Allocation);
    m_Buffer = VK_NULL_HANDLE;
}

void Buffer::SubmitForDeletion(DeletionQueue &p_Queue) const
{
    const VmaAllocator allocator = m_Info.Allocator;
    const VkBuffer buffer = m_Buffer;
    const VmaAllocation allocation = m_Info.Allocation;
    p_Queue.Push([=] { vmaDestroyBuffer(allocator, buffer, allocation); });
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

void Buffer::Write(const void *p_Data)
{
    TKIT_ASSERT(m_Data, "[VULKIT] Cannot copy to unmapped buffer");
    TKit::Memory::ForwardCopy(m_Data, p_Data, m_Info.Size);
}

void Buffer::Write(const void *p_Data, VkDeviceSize p_Size, const VkDeviceSize p_Offset)
{
    TKIT_ASSERT(m_Data, "[VULKIT] Cannot copy to unmapped buffer");
    TKIT_ASSERT(m_Info.Size >= p_Size + p_Offset, "[VULKIT] Buffer slice is smaller than the data size");

    std::byte *data = static_cast<std::byte *>(m_Data) + p_Offset;
    TKit::Memory::ForwardCopy(data, p_Data, p_Size);
}

void Buffer::Write(const VkCommandBuffer p_CommandBuffer, const Buffer &p_Source, const VkDeviceSize p_Size)
{
    VkBufferCopy copy{};
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = p_Size == VK_WHOLE_SIZE ? m_Info.Size : p_Size;

    TKIT_ASSERT(p_Source.GetInfo().Size >= copy.size, "[ONYX] Specified size exceeds source buffer size");
    TKIT_ASSERT(m_Info.Size >= copy.size, "[ONYX] Specified size exceeds destination buffer size");
    m_Device.Table->CmdCopyBuffer(p_CommandBuffer, p_Source, m_Buffer, 1, &copy);
}

Result<> Buffer::Write(CommandPool &p_Pool, VkQueue p_Queue, const Buffer &p_Source, const VkDeviceSize p_Size)
{
    const auto result1 = p_Pool.BeginSingleTimeCommands();
    if (!result1)
        return Result<>::Error(result1.GetError());

    const VkCommandBuffer commandBuffer = result1.GetValue();
    Write(commandBuffer, p_Source, p_Size);
    return p_Pool.EndSingleTimeCommands(commandBuffer, p_Queue);
}

void Buffer::WriteAt(const u32 p_Index, const void *p_Data)
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "[VULKIT] Index out of bounds");

    const VkDeviceSize size = m_Info.InstanceAlignedSize * p_Index;
    std::byte *data = static_cast<std::byte *>(m_Data) + size;
    TKit::Memory::ForwardCopy(data, p_Data, m_Info.InstanceSize);
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

void Buffer::BindAsVertexBuffer(const LogicalDevice::Proxy &p_Device, const VkCommandBuffer p_CommandBuffer,
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
