#include "vkit/core/pch.hpp"
#include "vkit/resource/host_buffer.hpp"

namespace VKit
{
HostBuffer::HostBuffer(const VkDeviceSize p_InstanceCount, const VkDeviceSize p_InstanceSize,
                       const VkDeviceSize p_Alignment)
    : m_InstanceCount(p_InstanceCount), m_InstanceSize(p_InstanceSize), m_Size(p_InstanceCount * p_InstanceSize)
{
    m_Data = TKit::Memory::AllocateAligned(m_Size, p_Alignment);
}

void HostBuffer::Write(const void *p_Data, BufferCopy p_Info)
{
    if (p_Info.Size == VK_WHOLE_SIZE)
        p_Info.Size = m_Size - p_Info.DstOffset;

    TKIT_ASSERT(m_Size >= p_Info.Size + p_Info.DstOffset,
                "[VULKIT][HOST-BUFFER] Buffer slice ({}) is smaller than the data size ({})", m_Size,
                p_Info.Size + p_Info.DstOffset);

    std::byte *dst = static_cast<std::byte *>(m_Data) + p_Info.DstOffset;
    const std::byte *src = static_cast<const std::byte *>(p_Data) + p_Info.SrcOffset;
    TKit::Memory::ForwardCopy(dst, src, p_Info.Size);
}

void HostBuffer::Write(const HostBuffer &p_Data, const BufferCopy &p_Info)
{
    const VkDeviceSize size = p_Info.Size == VK_WHOLE_SIZE ? (p_Data.GetSize() - p_Info.SrcOffset) : (p_Info.Size);
    Write(p_Data.GetData(), {.Size = size, .SrcOffset = p_Info.SrcOffset, .DstOffset = p_Info.DstOffset});
}

void HostBuffer::WriteAt(const u32 p_Index, const void *p_Data)
{
    TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_InstanceCount, "[VULKIT][HOST-BUFFER] ");

    const VkDeviceSize size = m_InstanceSize * p_Index;
    std::byte *data = static_cast<std::byte *>(m_Data) + size;
    TKit::Memory::ForwardCopy(data, p_Data, m_InstanceSize);
}

void HostBuffer::Resize(const VkDeviceSize p_InstanceCount)
{
    const VkDeviceSize size = p_InstanceCount * m_InstanceSize;
    void *data = TKit::Memory::AllocateAligned(size, m_Alignment);
    TKit::Memory::ForwardCopy(data, m_Data, m_Size);
    TKit::Memory::DeallocateAligned(m_Data);
    m_Size = size;
    m_InstanceCount = p_InstanceCount;
}

void HostBuffer::Destroy()
{
    TKit::Memory::DeallocateAligned(m_Data);
    m_Data = nullptr;
}

} // namespace VKit
