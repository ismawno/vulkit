#include "vkit/core/pch.hpp"
#include "vkit/buffer/buffer.hpp"

namespace VKit
{
static VkDeviceSize alignedSize(const VkDeviceSize p_Size, const VkDeviceSize p_Alignment) noexcept
{
    return (p_Size + p_Alignment - 1) & ~(p_Alignment - 1);
}

Buffer::Buffer(const Specs &p_Specs) noexcept
    : m_Allocator(p_Specs.Allocator), m_InstanceSize(p_Specs.InstanceSize),
      m_AlignedInstanceSize(alignedSize(p_Specs.InstanceSize, p_Specs.MinimumAlignment)),
      m_Size(m_AlignedInstanceSize * p_Specs.InstanceCount)
{
    createBuffer(p_Specs.Usage, p_Specs.AllocationInfo);
}

void Buffer::Destroy() noexcept
{
    vmaDestroyBuffer(m_Allocator, m_Buffer, m_Allocation);
}

void Buffer::SubmitForDeletion(DeletionQueue &p_Queue) noexcept
{
    const VmaAllocator allocator = m_Allocator;
    const VkBuffer buffer = m_Buffer;
    const VmaAllocation allocation = m_Allocation;
    p_Queue.Push([allocator, buffer, allocation]() { vmaDestroyBuffer(allocator, buffer, allocation); });
}

void Buffer::Map() noexcept
{
    if (m_Data)
        Unmap();
    TKIT_ASSERT_RETURNS(vmaMapMemory(m_Allocator, m_Allocation, &m_Data), VK_SUCCESS, "Failed to map buffer memory");
}

void Buffer::Unmap() noexcept
{
    if (!m_Data)
        return;
    vmaUnmapMemory(m_Allocator, m_Allocation);
    m_Data = nullptr;
}

void Buffer::Write(const void *p_Data, const VkDeviceSize p_Size, const VkDeviceSize p_Offset) noexcept
{
    TKIT_ASSERT(m_Data, "Cannot copy to unmapped buffer");
    TKIT_ASSERT((p_Size == VK_WHOLE_SIZE && p_Offset == 0) ||
                    (p_Size != VK_WHOLE_SIZE && (p_Offset + p_Size) <= m_Size),
                "Size + offset must be lower than the buffer size");
    if (p_Size == VK_WHOLE_SIZE)
        std::memcpy(m_Data, p_Data, m_Size);
    else
    {
        std::byte *offsetted = static_cast<std::byte *>(m_Data) + p_Offset;
        std::memcpy(offsetted, p_Data, p_Size);
    }
}
void Buffer::WriteAt(const usize p_Index, const void *p_Data) noexcept
{
    TKIT_ASSERT(p_Index < GetInstanceCount(), "Index out of bounds");
    Write(p_Data, m_InstanceSize, m_AlignedInstanceSize * p_Index);
}

void Buffer::Flush(const VkDeviceSize p_Size, const VkDeviceSize p_Offset) noexcept
{
    TKIT_ASSERT(m_Data, "Cannot flush unmapped buffer");
    TKIT_ASSERT_RETURNS(vmaFlushAllocation(m_Allocator, m_Allocation, p_Offset, p_Size), VK_SUCCESS,
                        "Failed to flush buffer memory");
}
void Buffer::FlushAt(const usize p_Index) noexcept
{
    TKIT_ASSERT(p_Index < GetInstanceCount(), "Index out of bounds");
    Flush(m_InstanceSize, m_AlignedInstanceSize * p_Index);
}

void Buffer::Invalidate(const VkDeviceSize p_Size, const VkDeviceSize p_Offset) noexcept
{
    TKIT_ASSERT(m_Data, "Cannot invalidate unmapped buffer");
    TKIT_ASSERT_RETURNS(vmaInvalidateAllocation(m_Allocator, m_Allocation, p_Offset, p_Size), VK_SUCCESS,
                        "Failed to invalidate buffer memory");
}
void Buffer::InvalidateAt(const usize p_Index) noexcept
{
    TKIT_ASSERT(p_Index < GetInstanceCount(), "Index out of bounds");
    Invalidate(m_InstanceSize, m_AlignedInstanceSize * p_Index);
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
    TKIT_ASSERT(p_Index < GetInstanceCount(), "Index out of bounds");
    return GetDescriptorInfo(m_InstanceSize, m_AlignedInstanceSize * p_Index);
}

void *Buffer::GetData() const noexcept
{
    return m_Data;
}
void *Buffer::ReadAt(const usize p_Index) const noexcept
{
    TKIT_ASSERT(p_Index < GetInstanceCount(), "Index out of bounds");
    return static_cast<std::byte *>(m_Data) + m_AlignedInstanceSize * p_Index;
}

void Buffer::CopyFrom(const Buffer &p_Source) noexcept
{
    TKIT_ASSERT(m_Size == p_Source.m_Size, "Cannot copy buffers of different sizes");

    VkCommandBuffer commandBuffer = m_Device->BeginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = m_Size;
    vkCmdCopyBuffer(commandBuffer, p_Source.m_Buffer, m_Buffer, 1, &copyRegion);

    m_Device->EndSingleTimeCommands(commandBuffer);
}

VkBuffer Buffer::GetBuffer() const noexcept
{
    return m_Buffer;
}
VkDeviceSize Buffer::GetSize() const noexcept
{
    return m_Size;
}
VkDeviceSize Buffer::GetInstanceSize() const noexcept
{
    return m_InstanceSize;
}
VkDeviceSize Buffer::GetInstanceCount() const noexcept
{
    return m_Size / m_AlignedInstanceSize;
}

void Buffer::createBuffer(const VkBufferUsageFlags p_Usage, const VmaAllocationCreateInfo &p_AllocationInfo) noexcept
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_Size;
    bufferInfo.usage = p_Usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    TKIT_ASSERT_RETURNS(vmaCreateBuffer(m_Allocator, &bufferInfo, &p_AllocationInfo, &m_Buffer, &m_Allocation, nullptr),
                        VK_SUCCESS, "Failed to create buffer");
}

} // namespace VKit