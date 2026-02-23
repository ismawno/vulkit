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
    VKIT_NO_DISCARD static Result<CommandPool> Create(const ProxyDevice &device, u32 queueFamilyIndex,
                                                      VkCommandPoolCreateFlags flags = 0);

    CommandPool() = default;
    CommandPool(const ProxyDevice &device, const VkCommandPool pool) : m_Device(device), m_Pool(pool)
    {
    }

    void Destroy();

    VKIT_NO_DISCARD Result<VkCommandBuffer> Allocate(
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    VKIT_NO_DISCARD Result<> Allocate(TKit::Span<VkCommandBuffer> commandBuffers,
                                      VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    void Deallocate(TKit::Span<const VkCommandBuffer> commandBuffers) const;
    VKIT_NO_DISCARD Result<> Reset(VkCommandPoolResetFlags flags = 0) const;
    VKIT_NO_DISCARD Result<VkCommandBuffer> BeginSingleTimeCommands() const;
    VKIT_NO_DISCARD Result<> EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue) const;

    template <typename F> VKIT_NO_DISCARD Result<> ImmediateSubmission(const VkQueue queue, F &&fun) const
    {
        const auto result = BeginSingleTimeCommands();
        TKIT_RETURN_ON_ERROR(result);

        const VkCommandBuffer cmd = result.GetValue();
        std::forward<F>(fun)(cmd);
        return EndSingleTimeCommands(cmd, queue);
    }

    VKIT_SET_DEBUG_NAME(m_Pool, VK_OBJECT_TYPE_COMMAND_POOL)

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
