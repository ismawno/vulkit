#include "vkit/core/pch.hpp"
#include "vkit/execution/command_pool.hpp"

namespace VKit
{
Result<CommandPool> CommandPool::Create(const ProxyDevice &device, const u32 queueFamilyIndex,
                                        const VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = queueFamilyIndex;
    createInfo.flags = flags;

    VkCommandPool pool;
    VKIT_RETURN_IF_FAILED(device.Table->CreateCommandPool(device, &createInfo, device.AllocationCallbacks, &pool),
                          Result<CommandPool>);

    return Result<CommandPool>::Ok(device, pool);
}

void CommandPool::Destroy()
{
    if (m_Pool)
    {
        m_Device.Table->DestroyCommandPool(m_Device, m_Pool, m_Device.AllocationCallbacks);
        m_Pool = VK_NULL_HANDLE;
    }
}

Result<> CommandPool::Allocate(const TKit::Span<VkCommandBuffer> commandBuffers, const VkCommandBufferLevel level) const
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = m_Pool;
    allocateInfo.level = level;
    allocateInfo.commandBufferCount = commandBuffers.GetSize();

    VKIT_RETURN_IF_FAILED(m_Device.Table->AllocateCommandBuffers(m_Device, &allocateInfo, commandBuffers.GetData()),
                          Result<>);
    return Result<>::Ok();
}
Result<VkCommandBuffer> CommandPool::Allocate(const VkCommandBufferLevel level) const
{
    VkCommandBuffer commandBuffer;
    TKIT_RETURN_IF_FAILED(Allocate(commandBuffer, level));
    return commandBuffer;
}

void CommandPool::Deallocate(const TKit::Span<const VkCommandBuffer> commandBuffers) const
{
    m_Device.Table->FreeCommandBuffers(m_Device, m_Pool, commandBuffers.GetSize(), commandBuffers.GetData());
}

Result<> CommandPool::Reset(const VkCommandPoolResetFlags flags) const
{
    VKIT_RETURN_IF_FAILED(m_Device.Table->ResetCommandPool(m_Device, m_Pool, flags), Result<>);
    return Result<>::Ok();
}

Result<VkCommandBuffer> CommandPool::BeginSingleTimeCommands() const
{
    const auto result = Allocate();
    TKIT_RETURN_ON_ERROR(result);

    const VkCommandBuffer commandBuffer = result.GetValue();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VKIT_RETURN_IF_FAILED(m_Device.Table->BeginCommandBuffer(commandBuffer, &beginInfo), Result<VkCommandBuffer>,
                          Deallocate(commandBuffer));

    return commandBuffer;
}

Result<> CommandPool::EndSingleTimeCommands(const VkCommandBuffer commandBuffer, const VkQueue queue) const
{
    VKIT_RETURN_IF_FAILED(m_Device.Table->EndCommandBuffer(commandBuffer), Result<>, Deallocate(commandBuffer));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VKIT_RETURN_IF_FAILED(m_Device.Table->QueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE), Result<>,
                          Deallocate(commandBuffer));

    VKIT_RETURN_IF_FAILED(m_Device.Table->QueueWaitIdle(queue), Result<>, Deallocate(commandBuffer));

    return Result<>::Ok();
}

} // namespace VKit
