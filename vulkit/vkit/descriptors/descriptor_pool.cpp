#include "vkit/core/pch.hpp"
#include "vkit/descriptors/descriptor_set.hpp"
#include "vkit/descriptors/descriptor_pool.hpp"

namespace VKit
{
DescriptorPool::Builder::Builder(const LogicalDevice::Proxy &p_Device) noexcept : m_Device(p_Device)
{
}

Result<DescriptorPool> DescriptorPool::Builder::Build() const noexcept
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCreateDescriptorPool, Result<DescriptorPool>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkDestroyDescriptorPool, Result<DescriptorPool>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkAllocateDescriptorSets, Result<DescriptorPool>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkFreeDescriptorSets, Result<DescriptorPool>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkResetDescriptorPool, Result<DescriptorPool>);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = m_PoolSizes.GetSize();
    poolInfo.pPoolSizes = m_PoolSizes.GetData();
    poolInfo.maxSets = m_MaxSets;
    poolInfo.flags = m_Flags;

    VkDescriptorPool pool;
    const VkResult result =
        m_Device.Table->CreateDescriptorPool(m_Device, &poolInfo, m_Device.AllocationCallbacks, &pool);
    if (result != VK_SUCCESS)
        return Result<DescriptorPool>::Error(result, "Failed to create descriptor pool");

    DescriptorPool::Info info{};
    info.MaxSets = m_MaxSets;
    info.PoolSizes = m_PoolSizes;

    return Result<DescriptorPool>::Ok(m_Device, pool, info);
}

DescriptorPool::DescriptorPool(const LogicalDevice::Proxy &p_Device, const VkDescriptorPool p_Pool,
                               const Info &p_Info) noexcept
    : m_Device(p_Device), m_Pool(p_Pool), m_Info(p_Info)
{
}

void DescriptorPool::Destroy() noexcept
{
    TKIT_ASSERT(m_Pool, "[VULKIT] The descriptor pool is a NULL handle");
    m_Device.Table->DestroyDescriptorPool(m_Device, m_Pool, m_Device.AllocationCallbacks);
    m_Pool = VK_NULL_HANDLE;
}
void DescriptorPool::SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
{
    const VkDescriptorPool pool = m_Pool;
    const LogicalDevice::Proxy device = m_Device;
    p_Queue.Push([pool, device]() { device.Table->DestroyDescriptorPool(device, pool, device.AllocationCallbacks); });
}

const DescriptorPool::Info &DescriptorPool::GetInfo() const noexcept
{
    return m_Info;
}

Result<DescriptorSet> DescriptorPool::Allocate(const VkDescriptorSetLayout p_Layout) const noexcept
{
    VkDescriptorSet set;
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &p_Layout;

    const VkResult result = m_Device.Table->AllocateDescriptorSets(m_Device, &allocInfo, &set);
    if (result != VK_SUCCESS)
        return Result<DescriptorSet>::Error(result, "Failed to allocate descriptor set");

    return DescriptorSet::Create(m_Device, set);
}

Result<> DescriptorPool::Deallocate(const TKit::Span<const VkDescriptorSet> p_Sets) const noexcept
{
    const VkResult result = m_Device.Table->FreeDescriptorSets(m_Device, m_Pool, p_Sets.GetSize(), p_Sets.GetData());
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to deallocate descriptor sets");
    return Result<>::Ok();
}

Result<> DescriptorPool::Deallocate(const VkDescriptorSet p_Set) const noexcept
{
    const VkResult result = m_Device.Table->FreeDescriptorSets(m_Device, m_Pool, 1, &p_Set);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to deallocate descriptor sets");
    return Result<>::Ok();
}

Result<> DescriptorPool::Reset() noexcept
{
    const VkResult result = m_Device.Table->ResetDescriptorPool(m_Device, m_Pool, 0);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to deallocate descriptor sets");
    return Result<>::Ok();
}

const LogicalDevice::Proxy &DescriptorPool::GetDevice() const noexcept
{
    return m_Device;
}
VkDescriptorPool DescriptorPool::GetHandle() const noexcept
{
    return m_Pool;
}
DescriptorPool::operator VkDescriptorPool() const noexcept
{
    return m_Pool;
}
DescriptorPool::operator bool() const noexcept
{
    return m_Pool != VK_NULL_HANDLE;
}

DescriptorPool::Builder &DescriptorPool::Builder::SetMaxSets(u32 p_MaxSets) noexcept
{
    m_MaxSets = p_MaxSets;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::SetFlags(VkDescriptorPoolCreateFlags p_Flags) noexcept
{
    m_Flags = p_Flags;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::AddFlags(VkDescriptorPoolCreateFlags p_Flags) noexcept
{
    m_Flags |= p_Flags;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::RemoveFlags(VkDescriptorPoolCreateFlags p_Flags) noexcept
{
    m_Flags &= ~p_Flags;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::AddPoolSize(VkDescriptorType p_Type, u32 p_Size) noexcept
{
    m_PoolSizes.Append(VkDescriptorPoolSize{p_Type, p_Size});
    return *this;
}

} // namespace VKit
