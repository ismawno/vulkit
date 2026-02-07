#include "vkit/core/pch.hpp"
#include "vkit/state/descriptor_set.hpp"
#include "vkit/state/descriptor_pool.hpp"

namespace VKit
{
Result<DescriptorPool> DescriptorPool::Builder::Build() const
{
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = m_PoolSizes.GetSize();
    poolInfo.pPoolSizes = m_PoolSizes.GetData();
    poolInfo.maxSets = m_MaxSets;
    poolInfo.flags = m_Flags;

    VkDescriptorPool pool;
    VKIT_RETURN_IF_FAILED(
        m_Device.Table->CreateDescriptorPool(m_Device, &poolInfo, m_Device.AllocationCallbacks, &pool),
        Result<DescriptorPool>);

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

Result<DescriptorSet> DescriptorPool::Allocate(const VkDescriptorSetLayout layout) const
{
    VkDescriptorSet set;
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VKIT_RETURN_IF_FAILED(m_Device.Table->AllocateDescriptorSets(m_Device, &allocInfo, &set), Result<DescriptorSet>);
    return DescriptorSet{m_Device, set};
}

Result<> DescriptorPool::Deallocate(const TKit::Span<const VkDescriptorSet> sets) const
{
    VKIT_RETURN_IF_FAILED(m_Device.Table->FreeDescriptorSets(m_Device, m_Pool, sets.GetSize(), sets.GetData()),
                          Result<>);
    return Result<>::Ok();
}

Result<> DescriptorPool::Reset(VkDescriptorPoolResetFlags flags)
{
    VKIT_RETURN_IF_FAILED(m_Device.Table->ResetDescriptorPool(m_Device, m_Pool, flags), Result<>);
    return Result<>::Ok();
}

DescriptorPool::Builder &DescriptorPool::Builder::SetMaxSets(u32 maxSets)
{
    m_MaxSets = maxSets;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::SetFlags(VkDescriptorPoolCreateFlags flags)
{
    m_Flags = flags;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::AddFlags(VkDescriptorPoolCreateFlags flags)
{
    m_Flags |= flags;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::RemoveFlags(VkDescriptorPoolCreateFlags flags)
{
    m_Flags &= ~flags;
    return *this;
}
DescriptorPool::Builder &DescriptorPool::Builder::AddPoolSize(VkDescriptorType type, u32 size)
{
    m_PoolSizes.Append(VkDescriptorPoolSize{type, size});
    return *this;
}

} // namespace VKit
