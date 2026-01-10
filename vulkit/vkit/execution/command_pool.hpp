#pragma once

#ifndef VKIT_ENABLE_COMMAND_POOL
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_COMMAND_POOL"
#endif

#include "vkit/device/proxy_device.hpp"
#include "tkit/container/span.hpp"

namespace VKit
{
class CommandPool
{
  public:
    VKIT_NO_DISCARD static Result<CommandPool> Create(const ProxyDevice &p_Device, u32 p_QueueFamilyIndex,
                                                      VkCommandPoolCreateFlags p_Flags = 0);

    CommandPool() = default;
    CommandPool(const ProxyDevice &p_Device, const VkCommandPool p_Pool) : m_Device(p_Device), m_Pool(p_Pool)
    {
    }

    void Destroy();

    VKIT_NO_DISCARD Result<VkCommandBuffer> Allocate(
        VkCommandBufferLevel p_Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    VKIT_NO_DISCARD Result<> Allocate(TKit::Span<VkCommandBuffer> p_CommandBuffers,
                                      VkCommandBufferLevel p_Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    void Deallocate(TKit::Span<const VkCommandBuffer> p_CommandBuffers) const;
    VKIT_NO_DISCARD Result<> Reset(VkCommandPoolResetFlags p_Flags = 0) const;
    VKIT_NO_DISCARD Result<VkCommandBuffer> BeginSingleTimeCommands() const;
    VKIT_NO_DISCARD Result<> EndSingleTimeCommands(VkCommandBuffer p_CommandBuffer, VkQueue p_Queue) const;

    const ProxyDevice &GetDevice() const
    {
        return m_Device;
    }

    VkCommandPool GetHandle() const
    {
        return m_Pool;
    }
    operator VkCommandPool() const
    {
        return m_Pool;
    }
    operator bool() const
    {
        return m_Pool != VK_NULL_HANDLE;
    }

  private:
    ProxyDevice m_Device{};
    VkCommandPool m_Pool = VK_NULL_HANDLE;
};
}; // namespace VKit
