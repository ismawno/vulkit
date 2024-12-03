#pragma once

#include "vkit/backend/logical_device.hpp"

namespace VKit
{
class CommandPool
{
  public:
    struct Specs
    {
        LogicalDevice::Proxy Device;
        u32 QueueFamilyIndex;
        VkCommandPoolCreateFlags Flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    };

    static Result<CommandPool> Create(const Specs &p_Specs) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) noexcept;

    Result<VkCommandBuffer> Allocate(VkCommandBufferLevel p_Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const noexcept;
    VulkanResult Allocate(std::span<VkCommandBuffer> p_CommandBuffers,
                          VkCommandBufferLevel p_Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const noexcept;

    void Deallocate(VkCommandBuffer p_CommandBuffer) const noexcept;
    void Deallocate(std::span<const VkCommandBuffer> p_CommandBuffers) const noexcept;

    Result<VkCommandBuffer> BeginSingleTimeCommands() const noexcept;
    VulkanResult EndSingleTimeCommands(VkCommandBuffer p_CommandBuffer, VkQueue p_Queue) const noexcept;

    VkCommandPool GetPool() const noexcept;
    explicit(false) operator VkCommandPool() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    CommandPool(const LogicalDevice::Proxy &p_Device, VkCommandPool p_Pool) noexcept;

    LogicalDevice::Proxy m_Device;
    VkCommandPool m_Pool;
};
}; // namespace VKit