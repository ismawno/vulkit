#include "vkit/core/pch.hpp"
#include "vkit/resource/host_buffer.hpp"
#include "tkit/memory/memory.hpp"

namespace VKit
{
HostBuffer::HostBuffer(const VkDeviceSize instanceCount, const VkDeviceSize instanceSize, const VkDeviceSize alignment)
    : m_InstanceCount(instanceCount), m_InstanceSize(instanceSize), m_Size(instanceCount * instanceSize),
      m_Alignment(alignment)
{
    m_Data = TKit::Memory::AllocateAligned(m_Size, alignment);
}

void HostBuffer::Write(const void *data, const VkBufferCopy &copy)
{
    TKIT_ASSERT(m_Size >= copy.size + copy.dstOffset,
                "[VULKIT][HOST-BUFFER] Copy size ({}) must be smaller or equal than the buffer size ({}) minus "
                "destination offset ({})",
                copy.size, m_Size, copy.dstOffset);

    std::byte *dst = static_cast<std::byte *>(m_Data) + copy.dstOffset;
    const std::byte *src = static_cast<const std::byte *>(data) + copy.srcOffset;
    TKit::Memory::ForwardCopy(dst, src, copy.size);
}

void HostBuffer::WriteAt(const u32 index, const void *pdata)
{
    TKIT_CHECK_OUT_OF_BOUNDS(index, m_InstanceCount, "[VULKIT][HOST-BUFFER] ");

    const VkDeviceSize size = m_InstanceSize * index;
    std::byte *data = static_cast<std::byte *>(m_Data) + size;
    TKit::Memory::ForwardCopy(data, pdata, m_InstanceSize);
}

void HostBuffer::Resize(const VkDeviceSize instanceCount)
{
    const VkDeviceSize size = instanceCount * m_InstanceSize;
    void *data = TKit::Memory::AllocateAligned(size, m_Alignment);
    TKit::Memory::ForwardCopy(data, m_Data, m_Size);
    TKit::Memory::DeallocateAligned(m_Data);
    m_Size = size;
    m_InstanceCount = instanceCount;
}

void HostBuffer::Destroy()
{
    TKit::Memory::DeallocateAligned(m_Data);
    m_Data = nullptr;
}

} // namespace VKit
