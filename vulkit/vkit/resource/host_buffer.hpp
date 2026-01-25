#pragma once

#ifndef VKIT_ENABLE_HOST_BUFFER
#    error                                                                                                             \
        "[VULKIT][HOST-BUFFER] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_HOST_BUFFER"
#endif

#include "vkit/core/alias.hpp"
#include "tkit/utils/debug.hpp"

namespace VKit
{
class HostBuffer
{
  public:
    HostBuffer() = default;
    HostBuffer(VkDeviceSize instanceCount, VkDeviceSize instanceSize,
               VkDeviceSize alignment = alignof(std::max_align_t));

    template <typename T> static HostBuffer Create(const VkDeviceSize instanceCount)
    {
        return HostBuffer(instanceCount, sizeof(T), alignof(T));
    }

    const void *ReadAt(const u32 index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, m_InstanceCount, "[VULKIT][HOST-BUFFER] ");
        return static_cast<std::byte *>(m_Data) + m_InstanceSize * index;
    }
    void *ReadAt(const u32 index)
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, m_InstanceCount, "[VULKIT][HOST-BUFFER] ");
        return static_cast<std::byte *>(m_Data) + m_InstanceSize * index;
    }

    void Write(const void *data, const VkBufferCopy &copy);
    void WriteAt(u32 index, const void *data);

    void Destroy();

    const void *GetData() const
    {
        return m_Data;
    }
    void *GetData()
    {
        return m_Data;
    }

    void Resize(VkDeviceSize instanceCount);

    operator bool() const
    {
        return m_Data != nullptr;
    }

    VkDeviceSize GetInstanceCount() const
    {
        return m_InstanceCount;
    }
    VkDeviceSize GetInstanceSize() const
    {
        return m_InstanceSize;
    }
    VkDeviceSize GetSize() const
    {
        return m_Size;
    }

  private:
    void *m_Data = nullptr;
    VkDeviceSize m_InstanceCount = 0;
    VkDeviceSize m_InstanceSize = 0;
    VkDeviceSize m_Size = 0;
    VkDeviceSize m_Alignment = 0;
};
} // namespace VKit
