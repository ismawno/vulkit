#include "vkit/core/pch.hpp"
#include "vkit/buffer/buffer.hpp"
#include "vkit/backend/command_pool.hpp"

namespace VKit
{
static VkDeviceSize alignedSize(const VkDeviceSize p_Size, const VkDeviceSize p_Alignment) noexcept
{
    return (p_Size + p_Alignment - 1) & ~(p_Alignment - 1);
}

RawResult<Buffer> Buffer::Create(const Specs &p_Specs) noexcept
{
    Info info{};
    info.Allocator = p_Specs.Allocator;
    info.InstanceSize = p_Specs.InstanceSize;
    info.InstanceCount = p_Specs.InstanceCount;
    info.AlignedInstanceSize = alignedSize(p_Specs.InstanceSize, p_Specs.MinimumAlignment);
    info.Size = info.AlignedInstanceSize * p_Specs.InstanceCount;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = info.Size;
    bufferInfo.usage = p_Specs.Usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer;
    const VkResult result =
        vmaCreateBuffer(p_Specs.Allocator, &bufferInfo, &p_Specs.AllocationInfo, &buffer, &info.Allocation, nullptr);
    if (result != VK_SUCCESS)
        return RawResult<Buffer>::Error(result, "Failed to create buffer");

    return RawResult<Buffer>::Ok(buffer, info);
}

Buffer::Buffer(const VkBuffer p_Buffer, const Info &p_Info) noexcept : m_Buffer(p_Buffer), m_Info(p_Info)
{
}

void Buffer::Destroy() noexcept
{
    vmaDestroyBuffer(m_Info.Allocator, m_Buffer, m_Info.Allocation);
}

void Buffer::SubmitForDeletion(DeletionQueue &p_Queue) noexcept
{
    const VmaAllocator allocator = m_Info.Allocator;
    const VkBuffer buffer = m_Buffer;
    const VmaAllocation allocation = m_Info.Allocation;
    p_Queue.Push([allocator, buffer, allocation]() { vmaDestroyBuffer(allocator, buffer, allocation); });
}

void Buffer::Map() noexcept
{
    if (m_Data)
        Unmap();
    TKIT_ASSERT_RETURNS(vmaMapMemory(m_Info.Allocator, m_Info.Allocation, &m_Data), VK_SUCCESS,
                        "Failed to map buffer memory");
}

void Buffer::Unmap() noexcept
{
    if (!m_Data)
        return;
    vmaUnmapMemory(m_Info.Allocator, m_Info.Allocation);
    m_Data = nullptr;
}

void Buffer::Write(const void *p_Data, const VkDeviceSize p_Size, const VkDeviceSize p_Offset) noexcept
{
    TKIT_ASSERT(m_Data, "Cannot copy to unmapped buffer");
    TKIT_ASSERT((p_Size == VK_WHOLE_SIZE && p_Offset == 0) ||
                    (p_Size != VK_WHOLE_SIZE && (p_Offset + p_Size) <= m_Info.Size),
                "Size + offset must be lower than the buffer size");
    if (p_Size == VK_WHOLE_SIZE)
        std::memcpy(m_Data, p_Data, m_Info.Size);
    else
    {
        std::byte *offsetted = static_cast<std::byte *>(m_Data) + p_Offset;
        std::memcpy(offsetted, p_Data, p_Size);
    }
}
void Buffer::WriteAt(const usize p_Index, const void *p_Data) noexcept
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "Index out of bounds");
    Write(p_Data, m_Info.InstanceSize, m_Info.AlignedInstanceSize * p_Index);
}

void Buffer::Flush(const VkDeviceSize p_Size, const VkDeviceSize p_Offset) noexcept
{
    TKIT_ASSERT(m_Data, "Cannot flush unmapped buffer");
    TKIT_ASSERT_RETURNS(vmaFlushAllocation(m_Info.Allocator, m_Info.Allocation, p_Offset, p_Size), VK_SUCCESS,
                        "Failed to flush buffer memory");
}
void Buffer::FlushAt(const usize p_Index) noexcept
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "Index out of bounds");
    Flush(m_Info.InstanceSize, m_Info.AlignedInstanceSize * p_Index);
}

void Buffer::Invalidate(const VkDeviceSize p_Size, const VkDeviceSize p_Offset) noexcept
{
    TKIT_ASSERT(m_Data, "Cannot invalidate unmapped buffer");
    TKIT_ASSERT_RETURNS(vmaInvalidateAllocation(m_Info.Allocator, m_Info.Allocation, p_Offset, p_Size), VK_SUCCESS,
                        "Failed to invalidate buffer memory");
}
void Buffer::InvalidateAt(const usize p_Index) noexcept
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "Index out of bounds");
    Invalidate(m_Info.InstanceSize, m_Info.AlignedInstanceSize * p_Index);
}

VkDescriptorBufferInfo Buffer::GetDescriptorInfo(const VkDeviceSize p_Size, const VkDeviceSize p_Offset) const noexcept
{
    VkDescriptorBufferInfo info{};
    info.buffer = m_Buffer;
    info.offset = p_Offset;
    info.range = p_Size;
    return info;
}
VkDescriptorBufferInfo Buffer::GetDescriptorInfoAt(const usize p_Index) const noexcept
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "Index out of bounds");
    return GetDescriptorInfo(m_Info.InstanceSize, m_Info.AlignedInstanceSize * p_Index);
}

void *Buffer::GetData() const noexcept
{
    return m_Data;
}
void *Buffer::ReadAt(const usize p_Index) const noexcept
{
    TKIT_ASSERT(p_Index < m_Info.InstanceCount, "Index out of bounds");
    return static_cast<std::byte *>(m_Data) + m_Info.AlignedInstanceSize * p_Index;
}

VulkanRawResult Buffer::CopyFrom(const Buffer &p_Source, CommandPool &p_Pool, const VkQueue p_Queue) noexcept
{
    TKIT_ASSERT(m_Info.Size == p_Source.m_Info.Size, "Cannot copy buffers of different sizes");
    const auto result1 = p_Pool.BeginSingleTimeCommands();
    if (!result1)
        return result1.GetError();

    const VkCommandBuffer commandBuffer = result1.GetValue();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = m_Info.Size;
    vkCmdCopyBuffer(commandBuffer, p_Source.m_Buffer, m_Buffer, 1, &copyRegion);

    return p_Pool.EndSingleTimeCommands(commandBuffer, p_Queue);
}

VkBuffer Buffer::GetBuffer() const noexcept
{
    return m_Buffer;
}
const Buffer::Info &Buffer::GetInfo() const noexcept
{
    return m_Info;
}

} // namespace VKit