#include "vkit/core/pch.hpp"
#include "vkit/descriptors/descriptor_pool.hpp"

namespace VKit
{
DescriptorPool::Builder::Builder(const LogicalDevice::Proxy &p_Device) noexcept : m_Device(p_Device)
{
}

Result<DescriptorPool> DescriptorPool::Builder::Build() const noexcept
{
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = m_PoolSizes.size();
    poolInfo.pPoolSizes = m_PoolSizes.data();
    poolInfo.maxSets = m_MaxSets;
    poolInfo.flags = m_Flags;

    VkDescriptorPool pool;
    const VkResult result = vkCreateDescriptorPool(m_Device, &poolInfo, m_Device.AllocationCallbacks, &pool);
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
    vkDestroyDescriptorPool(m_Device, m_Pool, m_Device.AllocationCallbacks);
    m_Pool = VK_NULL_HANDLE;
}
void DescriptorPool::SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
{
    const VkDescriptorPool pool = m_Pool;
    const LogicalDevice::Proxy device = m_Device;
    p_Queue.Push([pool, device]() { vkDestroyDescriptorPool(device, pool, device.AllocationCallbacks); });
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

    const VkResult result = vkAllocateDescriptorSets(m_Device, &allocInfo, &set);
    if (result != VK_SUCCESS)
        return Result<DescriptorSet>::Error(result, "Failed to allocate descriptor set");

    return Result<DescriptorSet>::Ok(set);
}

void DescriptorPool::Deallocate(const TKit::Span<const VkDescriptorSet> p_Sets) const noexcept
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

VkDescriptorPool DescriptorPool::GetPool() const noexcept
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
    m_PoolSizes.push_back(VkDescriptorPoolSize{p_Type, p_Size});
    return *this;
}

} // namespace VKit