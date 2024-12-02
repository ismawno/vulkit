#pragma once

#include "vkit/backend/logical_device.hpp"

namespace VKit
{
class CommandPool
{
  public:
    static RawResult<CommandPool> Create(
        const LogicalDevice &p_Device, u32 p_QueueFamilyIndex,
        VkCommandPoolCreateFlags p_Flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) noexcept;

    RawResult<VkCommandBuffer> Allocate(VkCommandBufferLevel p_Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const noexcept;
    VulkanRawResult Allocate(std::span<VkCommandBuffer> p_CommandBuffers,
                             VkCommandBufferLevel p_Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const noexcept;

    void Deallocate(VkCommandBuffer p_CommandBuffer) const noexcept;
    void Deallocate(std::span<const VkCommandBuffer> p_CommandBuffers) const noexcept;

    RawResult<VkCommandBuffer> BeginSingleTimeCommands() const noexcept;
    VulkanRawResult EndSingleTimeCommands(VkCommandBuffer p_CommandBuffer, VkQueue p_Queue) const noexcept;

  private:
    CommandPool(const LogicalDevice &p_Device, VkCommandPool p_Pool) noexcept;

    LogicalDevice m_Device;
    VkCommandPool m_Pool;
};
}; // namespace VKit