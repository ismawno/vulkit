#include "vkit/core/pch.hpp"
#include "vkit/execution/command_pool.hpp"
#include "tkit/utils/defer.hpp"

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
    VKIT_RETURN_IF_FAILED(device.Table->CreateCommandPool(device, &createInfo, device.AllocationCallbacks, &pool));

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

Result<> CommandPool::Allocate(const TKit::Span<VkCommandBuffer> cmds, const VkCommandBufferLevel level) const
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = m_Pool;
    allocateInfo.level = level;
    allocateInfo.commandBufferCount = cmds.GetSize();

    VKIT_RETURN_IF_FAILED(m_Device.Table->AllocateCommandBuffers(m_Device, &allocateInfo, cmds.GetData()));
    return Result<>::Ok();
}
Result<VkCommandBuffer> CommandPool::Allocate(const VkCommandBufferLevel level) const
{
    VkCommandBuffer cmd;
    TKIT_RETURN_IF_FAILED(Allocate(cmd, level));
    return cmd;
}

void CommandPool::Deallocate(const TKit::Span<const VkCommandBuffer> cmds) const
{
    m_Device.Table->FreeCommandBuffers(m_Device, m_Pool, cmds.GetSize(), cmds.GetData());
}

Result<> CommandPool::Reset(const VkCommandPoolResetFlags flags) const
{
    VKIT_RETURN_IF_FAILED(m_Device.Table->ResetCommandPool(m_Device, m_Pool, flags));
    return Result<>::Ok();
}

Result<VkCommandBuffer> CommandPool::BeginSingleTimeCommands() const
{
    TKIT_ERROR_DEFER_SCOPE();
    const auto result = Allocate();
    TKIT_RETURN_ON_ERROR(result);

    const VkCommandBuffer cmd = *result;
    TKIT_ERROR_DEFER(Deallocate(cmd));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VKIT_RETURN_IF_FAILED(m_Device.Table->BeginCommandBuffer(cmd, &beginInfo));

    TKIT_ERROR_DEFER_COMMIT();
    return cmd;
}

Result<> CommandPool::EndSingleTimeCommands(const VkCommandBuffer cmd, const VkQueue queue) const
{
    TKIT_DEFER(Deallocate(cmd));
    VKIT_RETURN_IF_FAILED(m_Device.Table->EndCommandBuffer(cmd));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VKIT_RETURN_IF_FAILED(m_Device.Table->QueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    VKIT_RETURN_IF_FAILED(m_Device.Table->QueueWaitIdle(queue));

    return Result<>::Ok();
}

} // namespace VKit
