#include "vkit/core/pch.hpp"
#include "vkit/descriptors/descriptor_pool.hpp"
#include "vkit/core/core.hpp"

namespace VKit
{
DescriptorPool::DescriptorPool(const Specs &p_Specs) noexcept
{
    m_Device = Core::GetDevice();
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<u32>(p_Specs.PoolSizes.size());
    poolInfo.pPoolSizes = p_Specs.PoolSizes.data();
    poolInfo.maxSets = p_Specs.MaxSets;
    poolInfo.flags = p_Specs.PoolFlags;

    TKIT_ASSERT_RETURNS(vkCreateDescriptorPool(m_Device->GetDevice(), &poolInfo, nullptr, &m_Pool), VK_SUCCESS,
                        "Failed to create descriptor pool");
}

DescriptorPool::~DescriptorPool() noexcept
{
    vkDestroyDescriptorPool(m_Device->GetDevice(), m_Pool, nullptr);
}

VkDescriptorSet DescriptorPool::Allocate(const VkDescriptorSetLayout p_Layout) const noexcept
{
    VkDescriptorSet set;
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &p_Layout;

    {
        std::scoped_lock lock(m_Mutex);
        TKIT_PROFILE_MARK_LOCK(m_Mutex);
        if (vkAllocateDescriptorSets(m_Device->GetDevice(), &allocInfo, &set) != VK_SUCCESS)
            return VK_NULL_HANDLE;
    }

    return set;
}

void DescriptorPool::Deallocate(const std::span<const VkDescriptorSet> p_Sets) const noexcept
{
    std::scoped_lock lock(m_Mutex);
    TKIT_PROFILE_MARK_LOCK(m_Mutex);
    vkFreeDescriptorSets(m_Device->GetDevice(), m_Pool, static_cast<u32>(p_Sets.size()), p_Sets.data());
}

void DescriptorPool::Deallocate(const VkDescriptorSet p_Set) const noexcept
{
    std::scoped_lock lock(m_Mutex);
    TKIT_PROFILE_MARK_LOCK(m_Mutex);
    vkFreeDescriptorSets(m_Device->GetDevice(), m_Pool, 1, &p_Set);
}

void DescriptorPool::Reset() noexcept
{
    vkResetDescriptorPool(m_Device->GetDevice(), m_Pool, 0);
}

} // namespace VKit