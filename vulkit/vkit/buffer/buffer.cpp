#include "vkit/core/pch.hpp"
#include "vkit/buffer/buffer.hpp"
#include "vkit/rendering/command_pool.hpp"

namespace VKit
{
static VkDeviceSize alignedSize(const VkDeviceSize p_Size, const VkDeviceSize p_Alignment)
{
    return (p_Size + p_Alignment - 1) & ~(p_Alignment - 1);
}

Result<Buffer> Buffer::Create(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs)
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCmdBindVertexBuffers, Result<Buffer>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCmdBindIndexBuffer, Result<Buffer>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCmdCopyBuffer, Result<Buffer>);

    Info info{};
    info.Allocator = p_Specs.Allocator;
    info.InstanceSize = p_Specs.InstanceSize;
    info.InstanceCount = p_Specs.InstanceCount;
    info.InstanceAlignedSize = alignedSize(p_Specs.InstanceSize, p_Specs.PerInstanceMinimumAlignment);
    info.Size = info.InstanceAlignedSize * p_Specs.InstanceCount;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = info.Size;
    bufferInfo.usage = p_Specs.Usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer;
    VmaAllocationInfo allocationInfo;
    const VkResult result = vmaCreateBuffer(p_Specs.Allocator, &bufferInfo, &p_Specs.AllocationInfo, &buffer,
                                            &info.Allocation, &allocationInfo);

    void *data = nullptr;
    if (p_Specs.AllocationInfo.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
        data = allocationInfo.pMappedData;

    if (result != VK_SUCCESS)
        return Result<Buffer>::Error(result, "Failed to create buffer");

    return Result<Buffer>::Ok(p_Device, buffer, info, data);
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

void Buffer::Map()
{
    TKIT_ASSERT(!m_Data, "[VULKIT] Buffer is already mapped");
    TKIT_ASSERT_RETURNS(vmaMapMemory(m_Info.Allocator, m_Info.Allocation, &m_Data), VK_SUCCESS,
                        "[VULKIT] Failed to map buffer memory");
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
    std::memcpy(m_Data, p_Data, m_Info.Size);
}

void Buffer::Write(const void *p_Data, VkDeviceSize p_Size, const VkDeviceSize p_Offset)
{
    TKIT_ASSERT(m_Data, "[VULKIT] Cannot copy to unmapped buffer");
    TKIT_ASSERT(m_Info.Size >= p_Size + p_Offset, "[VULKIT] Buffer slice is smaller than the data size");

    std::byte *data = static_cast<std::byte *>(m_Data) + p_Offset;
    std::memcpy(data, p_Data, p_Size);
}
void Buffer::WriteAt(const u32 p_Index, const void *p_Data)
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "[VULKIT] Index out of bounds");

    const VkDeviceSize size = m_Info.InstanceAlignedSize * p_Index;
    std::byte *data = static_cast<std::byte *>(m_Data) + size;
    std::memcpy(data, p_Data, m_Info.InstanceSize);
}

void Buffer::Flush(const VkDeviceSize p_Size, const VkDeviceSize p_Offset)
{
    TKIT_ASSERT(m_Data, "[VULKIT] Cannot flush unmapped buffer");
    TKIT_ASSERT_RETURNS(vmaFlushAllocation(m_Info.Allocator, m_Info.Allocation, p_Offset, p_Size), VK_SUCCESS,
                        "[VULKIT] Failed to flush buffer memory");
}
void Buffer::FlushAt(const u32 p_Index)
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "[VULKIT] Index out of bounds");
    Flush(m_Info.InstanceSize, m_Info.InstanceAlignedSize * p_Index);
}

void Buffer::Invalidate(const VkDeviceSize p_Size, const VkDeviceSize p_Offset)
{
    TKIT_ASSERT(m_Data, "[VULKIT] Cannot invalidate unmapped buffer");
    TKIT_ASSERT_RETURNS(vmaInvalidateAllocation(m_Info.Allocator, m_Info.Allocation, p_Offset, p_Size), VK_SUCCESS,
                        "[VULKIT] Failed to invalidate buffer memory");
}
void Buffer::InvalidateAt(const u32 p_Index)
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "[VULKIT] Index out of bounds");
    Invalidate(m_Info.InstanceSize, m_Info.InstanceAlignedSize * p_Index);
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

Result<> Buffer::DeviceCopy(const Buffer &p_Source, CommandPool &p_Pool, const VkQueue p_Queue)
{
    TKIT_ASSERT(m_Info.Size == p_Source.m_Info.Size, "[VULKIT] Cannot copy buffers of different sizes");
    const auto result1 = p_Pool.BeginSingleTimeCommands();
    if (!result1)
        return Result<>::Error(result1.GetError());

    const VkCommandBuffer commandBuffer = result1.GetValue();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = m_Info.Size;
    m_Device.Table->CmdCopyBuffer(commandBuffer, p_Source.m_Buffer, m_Buffer, 1, &copyRegion);

    return p_Pool.EndSingleTimeCommands(commandBuffer, p_Queue);
}

} // namespace VKit
