#pragma once

#ifndef VKIT_ENABLE_HOST_BUFFER
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_HOST_BUFFER"
#endif

#include "vkit/core/alias.hpp"
#include "vkit/resource/utils.hpp"
#include "tkit/container/span.hpp"

namespace VKit
{
class HostBuffer
{
  public:
    HostBuffer(VkDeviceSize p_InstanceCount, VkDeviceSize p_InstanceSize,
               VkDeviceSize p_Alignment = alignof(std::max_align_t));

    template <typename T> HostBuffer Create(const VkDeviceSize p_InstanceCount)
    {
        return HostBuffer(p_InstanceCount, sizeof(T), alignof(T));
    }

    const void *ReadAt(const u32 p_Index) const
    {
        TKIT_ASSERT(p_Index < m_InstanceCount, "[VULKIT] Index out of bounds");
        return static_cast<std::byte *>(m_Data) + m_InstanceSize * p_Index;
    }
    void *ReadAt(const u32 p_Index)
    {
        TKIT_ASSERT(p_Index < m_InstanceCount, "[VULKIT] Index out of bounds");
        return static_cast<std::byte *>(m_Data) + m_InstanceSize * p_Index;
    }

    void Write(const void *p_Data, BufferCopy p_Info = {});
    void Write(const HostBuffer &p_Data, const BufferCopy &p_Info = {});

    template <typename T> void Write(const TKit::Span<const T> p_Data, const BufferCopy &p_Info = {})
    {
        const VkDeviceSize size = p_Info.Size == VK_WHOLE_SIZE
                                      ? (p_Data.GetSize() * sizeof(T) - p_Info.SrcOffset * sizeof(T))
                                      : (p_Info.Size * sizeof(T));
        Write(p_Data.GetData(),
              {.Size = size, .SrcOffset = p_Info.SrcOffset * sizeof(T), .DstOffset = p_Info.DstOffset * sizeof(T)});
    }

    void WriteAt(u32 p_Index, const void *p_Data);

    void Destroy();

    const void *GetData() const
    {
        return m_Data;
    }
    void *GetData()
    {
        return m_Data;
    }

    void Resize(VkDeviceSize p_InstanceCount);

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
