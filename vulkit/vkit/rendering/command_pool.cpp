#include "vkit/core/pch.hpp"
#include "vkit/rendering/command_pool.hpp"

namespace VKit
{
Result<CommandPool> CommandPool::Create(const LogicalDevice::Proxy &p_Device, const u32 p_QueueFamilyIndex,
                                        const VkCommandPoolCreateFlags p_Flags)
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCreateCommandPool, Result<CommandPool>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkDestroyCommandPool, Result<CommandPool>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkAllocateCommandBuffers, Result<CommandPool>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkResetCommandPool, Result<CommandPool>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkBeginCommandBuffer, Result<CommandPool>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkEndCommandBuffer, Result<CommandPool>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkQueueSubmit, Result<CommandPool>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkQueueWaitIdle, Result<CommandPool>);

    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = p_QueueFamilyIndex;
    createInfo.flags = p_Flags;

    VkCommandPool pool;
    const VkResult result =
        p_Device.Table->CreateCommandPool(p_Device, &createInfo, p_Device.AllocationCallbacks, &pool);
    if (result != VK_SUCCESS)
        return Result<CommandPool>::Error(result, "Failed to create the command pool");

    return Result<CommandPool>::Ok(p_Device, pool);
}

CommandPool::CommandPool(const LogicalDevice::Proxy &p_Device, const VkCommandPool p_Pool)
    : m_Device(p_Device), m_Pool(p_Pool)
{
}

void CommandPool::Destroy()
{
    TKIT_ASSERT(m_Pool, "[VULKIT] The command pool is a NULL handle");
    LogicalDevice::WaitIdle(m_Device);
    m_Device.Table->DestroyCommandPool(m_Device, m_Pool, m_Device.AllocationCallbacks);
    m_Pool = VK_NULL_HANDLE;
}

void CommandPool::SubmitForDeletion(DeletionQueue &p_Queue) const
{
    const LogicalDevice::Proxy device = m_Device;
    const VkCommandPool pool = m_Pool;
    const VkAllocationCallbacks *alloc = m_Device.AllocationCallbacks;
    p_Queue.Push([=] {
        LogicalDevice::WaitIdle(device);
        device.Table->DestroyCommandPool(device, pool, alloc);
    });
}

Result<> CommandPool::Allocate(const TKit::Span<VkCommandBuffer> p_CommandBuffers,
                               const VkCommandBufferLevel p_Level) const
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = m_Pool;
    allocateInfo.level = p_Level;
    allocateInfo.commandBufferCount = p_CommandBuffers.GetSize();

    const VkResult result = m_Device.Table->AllocateCommandBuffers(m_Device, &allocateInfo, p_CommandBuffers.GetData());
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to allocate command buffers");
    return Result<>::Ok();
}
Result<VkCommandBuffer> CommandPool::Allocate(const VkCommandBufferLevel p_Level) const
{
    VkCommandBuffer commandBuffer;
    const TKit::Span<VkCommandBuffer> commandBuffers(&commandBuffer, 1);

    const Result<> result = Allocate(commandBuffers, p_Level);
    if (result)
        return Result<VkCommandBuffer>::Ok(commandBuffer);
    return Result<VkCommandBuffer>::Error(result.GetError());
}

void CommandPool::Deallocate(const TKit::Span<const VkCommandBuffer> p_CommandBuffers) const
{
    m_Device.Table->FreeCommandBuffers(m_Device, m_Pool, p_CommandBuffers.GetSize(), p_CommandBuffers.GetData());
}
void CommandPool::Deallocate(const VkCommandBuffer p_CommandBuffer) const
{
    const TKit::Span<const VkCommandBuffer> commandBuffers(&p_CommandBuffer, 1);
    Deallocate(commandBuffers);
}

Result<> CommandPool::Reset(const VkCommandPoolResetFlags p_Flags) const
{
    const VkResult result = m_Device.Table->ResetCommandPool(m_Device, m_Pool, p_Flags);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to reset command pool");
    return Result<>::Ok();
}

Result<VkCommandBuffer> CommandPool::BeginSingleTimeCommands() const
{
    const Result<VkCommandBuffer> result = Allocate();
    if (!result)
        return result;

    const VkCommandBuffer commandBuffer = result.GetValue();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    const VkResult vkresult = m_Device.Table->BeginCommandBuffer(commandBuffer, &beginInfo);
    if (vkresult != VK_SUCCESS)
        return Result<VkCommandBuffer>::Error(vkresult, "Failed to begin command buffer");

    return Result<VkCommandBuffer>::Ok(commandBuffer);
}

Result<> CommandPool::EndSingleTimeCommands(const VkCommandBuffer p_CommandBuffer, const VkQueue p_Queue) const
{
    VkResult result = m_Device.Table->EndCommandBuffer(p_CommandBuffer);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to end command buffer");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &p_CommandBuffer;

    result = m_Device.Table->QueueSubmit(p_Queue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to submit command buffer");

    result = m_Device.Table->QueueWaitIdle(p_Queue);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to wait for queue to idle");

    Deallocate(p_CommandBuffer);
    return Result<>::Ok();
}

const LogicalDevice::Proxy &CommandPool::GetDevice() const
{
    return m_Device;
}

VkCommandPool CommandPool::GetHandle() const
{
    return m_Pool;
}
CommandPool::operator VkCommandPool() const
{
    return m_Pool;
}
CommandPool::operator bool() const
{
    return m_Pool != VK_NULL_HANDLE;
}

} // namespace VKit
