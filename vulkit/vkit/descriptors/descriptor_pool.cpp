#include "vkit/core/pch.hpp"
#include "vkit/descriptors/descriptor_pool.hpp"

namespace VKit
{
DescriptorPool::Builder::Builder(const LogicalDevice *p_Device) noexcept : m_Device(p_Device)
{
}

Result<DescriptorPool> DescriptorPool::Builder::Build() const noexcept
{
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<u32>(m_PoolSizes.size());
    poolInfo.pPoolSizes = m_PoolSizes.data();
    poolInfo.maxSets = m_MaxSets;
    poolInfo.flags = m_Flags;

    VkDescriptorPool pool;
    const VkResult result = vkCreateDescriptorPool(m_Device->GetDevice(), &poolInfo,
                                                   m_Device->GetInstance().GetInfo().AllocationCallbacks, &pool);
    if (result != VK_SUCCESS)
        return Result<DescriptorPool>::Error(result, "Failed to create descriptor pool");

    return Result<DescriptorPool>::Ok(*m_Device, pool);
}

DescriptorPool::DescriptorPool(const LogicalDevice &p_Device, const VkDescriptorPool p_Pool,
                               const Info &p_Info) noexcept
    : m_Device(p_Device), m_Pool(p_Pool), m_Info(p_Info)
{
}

void DescriptorPool::Destroy() noexcept
{
    vkDestroyDescriptorPool(m_Device.GetDevice(), m_Pool, m_Device.GetInstance().GetInfo().AllocationCallbacks);
}
void DescriptorPool::SubmitForDeletion(DeletionQueue &p_Queue) noexcept
{
    const VkDescriptorPool pool = m_Pool;
    const VkDevice device = m_Device.GetDevice();
    const VkAllocationCallbacks *alloc = m_Device.GetInstance().GetInfo().AllocationCallbacks;
    p_Queue.Push([pool, device, alloc]() { vkDestroyDescriptorPool(device, pool, alloc); });
}

const DescriptorPool::Info &DescriptorPool::GetInfo() const noexcept
{
    return m_Info;
}

Result<VkDescriptorSet> DescriptorPool::Allocate(const VkDescriptorSetLayout p_Layout) const noexcept
{
    VkDescriptorSet set;
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &p_Layout;

    const VkResult result = vkAllocateDescriptorSets(m_Device, &allocInfo, &set);
    if (result != VK_SUCCESS)
        return Result<VkDescriptorSet>::Error(result, "Failed to allocate descriptor set");

    return Result<VkDescriptorSet>::Ok(set);
}

void DescriptorPool::Deallocate(const std::span<const VkDescriptorSet> p_Sets) const noexcept
{
    vkFreeDescriptorSets(m_Device, m_Pool, static_cast<u32>(p_Sets.size()), p_Sets.data());
}

void DescriptorPool::Deallocate(const VkDescriptorSet p_Set) const noexcept
{
    vkFreeDescriptorSets(m_Device, m_Pool, 1, &p_Set);
}

void DescriptorPool::Reset() noexcept
{
    vkResetDescriptorPool(m_Device, m_Pool, 0);
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
    m_PoolSizes.push_back({p_Type, p_Size});
    return *this;
}

} // namespace VKit