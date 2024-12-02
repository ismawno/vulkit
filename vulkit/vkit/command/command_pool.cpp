#include "vkit/core/pch.hpp"
#include "vkit/command/command_pool.hpp"

namespace VKit
{
RawResult<CommandPool> CommandPool::Create(const LogicalDevice &p_Device, const u32 p_QueueFamilyIndex,
                                           const VkCommandPoolCreateFlags p_Flags) noexcept
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = p_QueueFamilyIndex;
    createInfo.flags = p_Flags;

    VkCommandPool pool;
    const VkResult result = vkCreateCommandPool(p_Device, &createInfo, nullptr, &pool);
    if (result != VK_SUCCESS)
        return RawResult<CommandPool>::Error(result, "Failed to create the command pool");

    return RawResult<CommandPool>::Ok(p_Device, pool);
}

CommandPool::CommandPool(const LogicalDevice &p_Device, const VkCommandPool p_Pool) noexcept
    : m_Device(p_Device), m_Pool(p_Pool)
{
}

void CommandPool::Destroy() noexcept
{
    LogicalDevice::WaitIdle(m_Device);
    vkDestroyCommandPool(m_Device, m_Pool, m_Device.GetInstance().GetInfo().AllocationCallbacks);
    m_Pool = VK_NULL_HANDLE;
}

void CommandPool::SubmitForDeletion(DeletionQueue &p_Queue) noexcept
{
    const VkDevice device = m_Device;
    const VkCommandPool pool = m_Pool;
    const VkAllocationCallbacks *alloc = m_Device.GetInstance().GetInfo().AllocationCallbacks;
    p_Queue.Push([device, pool, alloc]() {
        LogicalDevice::WaitIdle(device);
        vkDestroyCommandPool(device, pool, alloc);
    });
}

VulkanRawResult CommandPool::Allocate(const std::span<VkCommandBuffer> p_CommandBuffers,
                                      const VkCommandBufferLevel p_Level) const noexcept
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = m_Pool;
    allocateInfo.level = p_Level;
    allocateInfo.commandBufferCount = static_cast<u32>(p_CommandBuffers.size());

    const VkResult result = vkAllocateCommandBuffers(m_Device, &allocateInfo, p_CommandBuffers.data());
    if (result != VK_SUCCESS)
        return VulkanRawResult::Error(result, "Failed to allocate command buffers");
    return VulkanRawResult::Success();
}
RawResult<VkCommandBuffer> CommandPool::Allocate(const VkCommandBufferLevel p_Level) const noexcept
{
    VkCommandBuffer commandBuffer;
    const std::span<VkCommandBuffer> commandBuffers(&commandBuffer, 1);

    const VulkanRawResult result = Allocate(commandBuffers, p_Level);
    if (result)
        return RawResult<VkCommandBuffer>::Ok(commandBuffer);
    return RawResult<VkCommandBuffer>::Error(result);
}

void CommandPool::Deallocate(const std::span<const VkCommandBuffer> p_CommandBuffers) const noexcept
{
    vkFreeCommandBuffers(m_Device, m_Pool, static_cast<u32>(p_CommandBuffers.size()), p_CommandBuffers.data());
}
void CommandPool::Deallocate(const VkCommandBuffer p_CommandBuffer) const noexcept
{
    const std::span<const VkCommandBuffer> commandBuffers(&p_CommandBuffer, 1);
    Deallocate(commandBuffers);
}

RawResult<VkCommandBuffer> CommandPool::BeginSingleTimeCommands() const noexcept
{
    const RawResult<VkCommandBuffer> result = Allocate();
    if (!result)
        return result;

    const VkCommandBuffer commandBuffer = result.GetValue();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    const VkResult vkresult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (vkresult != VK_SUCCESS)
        return RawResult<VkCommandBuffer>::Error(vkresult, "Failed to begin command buffer");

    return RawResult<VkCommandBuffer>::Ok(commandBuffer);
}

VulkanRawResult CommandPool::EndSingleTimeCommands(const VkCommandBuffer p_CommandBuffer,
                                                   const VkQueue p_Queue) const noexcept
{
    VkResult result = vkEndCommandBuffer(p_CommandBuffer);
    if (result != VK_SUCCESS)
        return VulkanRawResult::Error(result, "Failed to end command buffer");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &p_CommandBuffer;

    result = vkQueueSubmit(p_Queue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
        return VulkanRawResult::Error(result, "Failed to submit command buffer");

    result = vkQueueWaitIdle(p_Queue);
    if (result != VK_SUCCESS)
        return VulkanRawResult::Error(result, "Failed to wait for queue to idle");

    Deallocate(p_CommandBuffer);
    return VulkanRawResult::Success();
}

} // namespace VKit