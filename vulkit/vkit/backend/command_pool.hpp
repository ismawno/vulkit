#pragma once

#include "vkit/backend/logical_device.hpp"

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

    static Result<CommandPool> Create(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs) noexcept;

    CommandPool() noexcept = default;
    CommandPool(const LogicalDevice::Proxy &p_Device, VkCommandPool p_Pool) noexcept;
    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

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
    LogicalDevice::Proxy m_Device{};
    VkCommandPool m_Pool = VK_NULL_HANDLE;
};
}; // namespace VKit