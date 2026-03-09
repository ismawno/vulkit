#include "vkit/core/pch.hpp"
#include "vkit/resource/host_buffer.hpp"
#include "tkit/memory/memory.hpp"

namespace VKit
{
HostBuffer::HostBuffer(const VkDeviceSize size, const VkDeviceSize alignment) : m_Size(size), m_Alignment(alignment)
{
    m_Data = TKit::AllocateAligned(m_Size, alignment);
}

void HostBuffer::Write(const void *data, const VkBufferCopy &copy)
{
    TKIT_ASSERT(m_Size >= copy.size + copy.dstOffset,
                "[VULKIT][HOST-BUFFER] Copy size ({}) must be smaller or equal than the buffer size ({}) minus "
                "destination offset ({})",
                copy.size, m_Size, copy.dstOffset);

    std::byte *dst = scast<std::byte *>(m_Data) + copy.dstOffset;
    const std::byte *src = scast<const std::byte *>(data) + copy.srcOffset;
    TKit::ForwardCopy(dst, src, copy.size);
}

void HostBuffer::Resize(const VkDeviceSize size)
{
    void *data = TKit::AllocateAligned(size, m_Alignment);
    TKit::ForwardCopy(data, m_Data, m_Size);
    TKit::DeallocateAligned(m_Data);
    m_Data = data;
    m_Size = size;
}

void HostBuffer::Destroy()
{
    TKit::DeallocateAligned(m_Data);
    m_Data = nullptr;
}

} // namespace VKit
