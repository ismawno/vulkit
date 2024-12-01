#pragma once

#include "vkit/buffer/device_buffer.hpp"
#include "vkit/core/dimension.hpp"
#include "vkit/draw/vertex.hpp"

namespace VKit
{
template <Dimension D> class VKIT_API VertexBuffer : public DeviceBuffer<Vertex<D>>
{
    TKIT_NON_COPYABLE(VertexBuffer)
  public:
    explicit VertexBuffer(const std::span<const Vertex<D>> p_Vertices) noexcept;

    void Bind(VkCommandBuffer p_CommandBuffer, VkDeviceSize p_Offset = 0) const noexcept;
};

template <Dimension D> class VKIT_API MutableVertexBuffer : public Buffer
{
    TKIT_NON_COPYABLE(MutableVertexBuffer)
  public:
    explicit MutableVertexBuffer(const std::span<const Vertex<D>> p_Vertices) noexcept;
    explicit MutableVertexBuffer(usize p_Size) noexcept;

    void Bind(VkCommandBuffer p_CommandBuffer, VkDeviceSize p_Offset = 0) const noexcept;

    void Write(std::span<const Vertex<D>> p_Vertices);
};
} // namespace VKit