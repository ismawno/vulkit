#pragma once

#include "vkit/backend/logical_device.hpp"

namespace VKit
{
/**
 * @brief Manages Vulkan command pools and their associated command buffers.
 *
 * Provides functionality for creating, allocating, deallocating, and managing
 * command buffers. Supports single-time commands for temporary operations.
 */
class VKIT_API CommandPool
{
  public:
    struct Specs
    {
        u32 QueueFamilyIndex;
        VkCommandPoolCreateFlags Flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    };

    /**
     * @brief Creates a Vulkan command pool with the specified settings.
     *
     * Initializes a command pool based on the provided device and specifications.
     *
     * @param p_Device The logical device proxy to create the command pool on.
     * @param p_Specs The specifications for the command pool.
     * @return A `Result` containing the created `CommandPool` or an error.
     */
    static Result<CommandPool> Create(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs) noexcept;

    CommandPool() noexcept = default;
    CommandPool(const LogicalDevice::Proxy &p_Device, VkCommandPool p_Pool) noexcept;
    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    /**
     * @brief Allocates a Vulkan command buffer from the pool.
     *
     * Creates a new command buffer at the specified level.
     *
     * @param p_Level The level of the command buffer (primary or secondary).
     * @return A `Result` containing the allocated command buffer or an error.
     */
    Result<VkCommandBuffer> Allocate(VkCommandBufferLevel p_Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const noexcept;

    /**
     * @brief Allocates multiple Vulkan command buffers from the pool.
     *
     * Creates a new command buffer for each element in the provided span.
     *
     * @param p_CommandBuffers The span of command buffers to allocate.
     * @param p_Level The level of the command buffers (primary or secondary).
     * @return A VulkanResult indicating success or failure.
     */
    VulkanResult Allocate(TKit::Span<VkCommandBuffer> p_CommandBuffers,
                          VkCommandBufferLevel p_Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const noexcept;

    /**
     * @brief Deallocates a Vulkan command buffer from the pool.
     *
     * Frees the specified command buffer back to the pool.
     *
     * @param p_CommandBuffer The command buffer to deallocate.
     */
    void Deallocate(VkCommandBuffer p_CommandBuffer) const noexcept;

    /**
     * @brief Deallocates multiple Vulkan command buffers from the pool.
     *
     * Frees each command buffer in the provided span back to the pool.
     *
     * @param p_CommandBuffers The span of command buffers to deallocate.
     */
    void Deallocate(TKit::Span<const VkCommandBuffer> p_CommandBuffers) const noexcept;

    /**
     * @brief Begins a single-time command operation.
     *
     * Allocates and starts recording a command buffer intended for temporary use.
     * The command buffer is expected to be submitted and discarded after execution.
     *
     * @return A `Result` containing the allocated and started command buffer or an error.
     */
    Result<VkCommandBuffer> BeginSingleTimeCommands() const noexcept;

    /**
     * @brief Ends a single-time command operation.
     *
     * Submits the recorded command buffer to the specified queue and cleans up the buffer.
     *
     * @param p_CommandBuffer The command buffer to submit and clean up.
     * @param p_Queue The queue to submit the command buffer to.
     * @return A VulkanResult indicating success or failure.
     */
    VulkanResult EndSingleTimeCommands(VkCommandBuffer p_CommandBuffer, VkQueue p_Queue) const noexcept;

    VkCommandPool GetPool() const noexcept;
    explicit(false) operator VkCommandPool() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkCommandPool m_Pool = VK_NULL_HANDLE;
};
}; // namespace VKit