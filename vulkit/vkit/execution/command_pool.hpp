#pragma once

#ifndef VKIT_ENABLE_COMMAND_POOL
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_COMMAND_POOL"
#endif

#include "vkit/device/logical_device.hpp"

namespace VKit
{
class VKIT_API CommandPool
{
  public:
    struct Specs
    {
        u32 QueueFamilyIndex;
        VkCommandPoolCreateFlags Flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    };

    static Result<CommandPool> Create(const LogicalDevice::Proxy &p_Device, u32 p_QueueFamilyIndex,
                                      VkCommandPoolCreateFlags p_Flags = 0);

    CommandPool() = default;
    CommandPool(const LogicalDevice::Proxy &p_Device, const VkCommandPool p_Pool) : m_Device(p_Device), m_Pool(p_Pool)
    {
    }

    void Destroy();

    Result<VkCommandBuffer> Allocate(VkCommandBufferLevel p_Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    Result<> Allocate(TKit::Span<VkCommandBuffer> p_CommandBuffers,
                      VkCommandBufferLevel p_Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    void Deallocate(VkCommandBuffer p_CommandBuffer) const;
    void Deallocate(TKit::Span<const VkCommandBuffer> p_CommandBuffers) const;
    Result<> Reset(VkCommandPoolResetFlags p_Flags = 0) const;
    Result<VkCommandBuffer> BeginSingleTimeCommands() const;
    Result<> EndSingleTimeCommands(VkCommandBuffer p_CommandBuffer, VkQueue p_Queue) const;

    const LogicalDevice::Proxy &GetDevice() const
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
    LogicalDevice::Proxy m_Device{};
    VkCommandPool m_Pool = VK_NULL_HANDLE;
};
}; // namespace VKit
