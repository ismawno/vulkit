#include "vkit/core/pch.hpp"
#include "vkit/state/descriptor_set.hpp"
#include "vkit/state/descriptor_pool.hpp"

namespace VKit
{

Result<DescriptorPool> DescriptorPool::Builder::Build() const
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
        return Result<DescriptorPool>::Error(result);

    DescriptorPool::Info info{};
    info.MaxSets = m_MaxSets;
    info.PoolSizes = m_PoolSizes;

    return Result<DescriptorPool>::Ok(m_Device, pool, info);
}

void DescriptorPool::Destroy()
{
    if (m_Pool)
    {
        m_Device.Table->DestroyDescriptorPool(m_Device, m_Pool, m_Device.AllocationCallbacks);
        m_Pool = VK_NULL_HANDLE;
    }
}

Result<DescriptorSet> DescriptorPool::Allocate(const VkDescriptorSetLayout p_Layout) const
{
    VkDescriptorSet set;
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &p_Layout;

    const VkResult result = m_Device.Table->AllocateDescriptorSets(m_Device, &allocInfo, &set);
    if (result != VK_SUCCESS)
        return Result<DescriptorSet>::Error(result);

    return DescriptorSet::Create(m_Device, set);
}

Result<> DescriptorPool::Deallocate(const TKit::Span<const VkDescriptorSet> p_Sets) const
{
    const VkResult result = m_Device.Table->FreeDescriptorSets(m_Device, m_Pool, p_Sets.GetSize(), p_Sets.GetData());
    if (result != VK_SUCCESS)
        return Result<>::Error(result);
    return Result<>::Ok();
}

Result<> DescriptorPool::Deallocate(const VkDescriptorSet p_Set) const
{
    const VkResult result = m_Device.Table->FreeDescriptorSets(m_Device, m_Pool, 1, &p_Set);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);
    return Result<>::Ok();
}

Result<> DescriptorPool::Reset()
{
    const VkResult result = m_Device.Table->ResetDescriptorPool(m_Device, m_Pool, 0);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);
    return Result<>::Ok();
}

DescriptorPool::Builder &DescriptorPool::Builder::SetMaxSets(u32 p_MaxSets)
{
    m_MaxSets = p_MaxSets;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::SetFlags(VkDescriptorPoolCreateFlags p_Flags)
{
    m_Flags = p_Flags;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::AddFlags(VkDescriptorPoolCreateFlags p_Flags)
{
    m_Flags |= p_Flags;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::RemoveFlags(VkDescriptorPoolCreateFlags p_Flags)
{
    m_Flags &= ~p_Flags;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::AddPoolSize(VkDescriptorType p_Type, u32 p_Size)
{
    m_PoolSizes.Append(VkDescriptorPoolSize{p_Type, p_Size});
    return *this;
}

} // namespace VKit
