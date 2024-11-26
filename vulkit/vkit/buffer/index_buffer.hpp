#pragma once

#include "vkit/buffer/device_buffer.hpp"
#include "vkit/core/dimension.hpp"

// AS OF RIGHT NOW THIS CANNOT BE CHANGED because VK_INDEX_TYPE_UINT32 is currently hardcoded in Bind()
#ifndef VKIT_INDEX_TYPE
#    define VKIT_INDEX_TYPE u32
#endif

namespace VKit
{
using Index = VKIT_INDEX_TYPE;
class VKIT_API IndexBuffer : public DeviceBuffer<Index>
{
    TKIT_NON_COPYABLE(IndexBuffer)
  public:
    IndexBuffer(const std::span<const Index> p_Vertices) noexcept;

    void Bind(VkCommandBuffer p_CommandBuffer, VkDeviceSize p_Offset = 0) const noexcept;
};

class VKIT_API MutableIndexBuffer : public Buffer
{
    TKIT_NON_COPYABLE(MutableIndexBuffer)
  public:
    MutableIndexBuffer(const std::span<const Index> p_Vertices) noexcept;
    MutableIndexBuffer(usize p_Size) noexcept;

    void Bind(VkCommandBuffer p_CommandBuffer, VkDeviceSize p_Offset = 0) const noexcept;

    void Write(std::span<const Index> p_Vertices);
};

} // namespace VKit