#pragma once

#ifndef VKIT_ENABLE_HOST_BUFFER
#    error                                                                                                             \
        "[VULKIT][HOST-BUFFER] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_HOST_BUFFER"
#endif

#include "vkit/core/alias.hpp"

namespace VKit
{
class HostBuffer
{
  public:
    HostBuffer() = default;
    HostBuffer(VkDeviceSize size, VkDeviceSize alignment = alignof(std::max_align_t));

    template <typename T> static HostBuffer Create(const u32 instanceCount)
    {
        return HostBuffer(instanceCount * sizeof(T), alignof(T));
    }

    void Write(const void *data, const VkBufferCopy &copy);
    void Destroy();

    const void *GetData() const
    {
        return m_Data;
    }
    void *GetData()
    {
        return m_Data;
    }

    void Resize(VkDeviceSize size);

    operator bool() const
    {
        return m_Data != nullptr;
    }

    VkDeviceSize GetSize() const
    {
        return m_Size;
    }

  private:
    void *m_Data = nullptr;
    VkDeviceSize m_Size = 0;
    VkDeviceSize m_Alignment = 0;
};
} // namespace VKit
