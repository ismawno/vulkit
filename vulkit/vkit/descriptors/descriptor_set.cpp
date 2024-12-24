#include "vkit/core/pch.hpp"
#include "vkit/descriptors/descriptor_set.hpp"
#include "vkit/buffer/buffer.hpp"

namespace VKit
{
DescriptorSet::DescriptorSet(const VkDescriptorSet p_Set) noexcept : m_Set(p_Set)
{
}

void DescriptorSet::Bind(const VkCommandBuffer p_CommandBuffer, const VkPipelineBindPoint p_BindPoint,
                         const VkPipelineLayout p_Layout, const std::span<const u32> p_DynamicOffsets) const noexcept
{
    Bind(p_CommandBuffer, m_Set, p_BindPoint, p_Layout, 0, p_DynamicOffsets);
}
void DescriptorSet::Bind(const VkCommandBuffer p_CommandBuffer, const std::span<const VkDescriptorSet> p_Sets,
                         const VkPipelineBindPoint p_BindPoint, const VkPipelineLayout p_Layout, const u32 p_FirstSet,
                         const std::span<const u32> p_DynamicOffsets) noexcept
{
    if (!p_DynamicOffsets.empty())
        vkCmdBindDescriptorSets(p_CommandBuffer, p_BindPoint, p_Layout, p_FirstSet, static_cast<u32>(p_Sets.size()),
                                p_Sets.data(), static_cast<u32>(p_DynamicOffsets.size()), p_DynamicOffsets.data());
    else
        vkCmdBindDescriptorSets(p_CommandBuffer, p_BindPoint, p_Layout, p_FirstSet, static_cast<u32>(p_Sets.size()),
                                p_Sets.data(), 0, nullptr);
}
void DescriptorSet::Bind(const VkCommandBuffer p_CommandBuffer, const VkDescriptorSet p_Set,
                         const VkPipelineBindPoint p_BindPoint, const VkPipelineLayout p_Layout, const u32 p_FirstSet,
                         const std::span<const u32> p_DynamicOffsets) noexcept
{
    if (!p_DynamicOffsets.empty())
        vkCmdBindDescriptorSets(p_CommandBuffer, p_BindPoint, p_Layout, p_FirstSet, 1, &p_Set,
                                static_cast<u32>(p_DynamicOffsets.size()), p_DynamicOffsets.data());
    else
        vkCmdBindDescriptorSets(p_CommandBuffer, p_BindPoint, p_Layout, p_FirstSet, 1, &p_Set, 0, nullptr);
}

VkDescriptorSet DescriptorSet::GetDescriptorSet() const noexcept
{
    return m_Set;
}
DescriptorSet::operator VkDescriptorSet() const noexcept
{
    return m_Set;
}
DescriptorSet::operator bool() const noexcept
{
    return m_Set != VK_NULL_HANDLE;
}

DescriptorSet::Writer::Writer(const LogicalDevice::Proxy &p_Device, const DescriptorSetLayout *p_Layout) noexcept
    : m_Device(p_Device), m_Layout(p_Layout)
{
}

void DescriptorSet::Writer::WriteBuffer(const u32 p_Binding, const VkDescriptorBufferInfo &p_BufferInfo) noexcept
{
    const VkDescriptorSetLayoutBinding &description = m_Layout->GetBindings()[p_Binding];

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = description.descriptorType;
    write.dstBinding = p_Binding;
    write.pBufferInfo = &p_BufferInfo;

    // I am not sure if this is correct!!
    write.descriptorCount = description.descriptorCount;

    m_Writes.push_back(write);
}

void DescriptorSet::Writer::WriteBuffer(const u32 p_Binding, const Buffer &p_Buffer) noexcept
{
    WriteBuffer(p_Binding, p_Buffer.GetDescriptorInfo());
}

void DescriptorSet::Writer::WriteImage(const u32 p_Binding, const VkDescriptorImageInfo &p_ImageInfo) noexcept
{
    const VkDescriptorSetLayoutBinding &description = m_Layout->GetBindings()[p_Binding];

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = description.descriptorType;
    write.dstBinding = p_Binding;
    write.pImageInfo = &p_ImageInfo;

    // I am not sure if this is correct!!
    write.descriptorCount = description.descriptorCount;

    m_Writes.push_back(write);
}

void DescriptorSet::Writer::Overwrite(const VkDescriptorSet p_Set) noexcept
{
    for (VkWriteDescriptorSet &write : m_Writes)
        write.dstSet = p_Set;
    vkUpdateDescriptorSets(m_Device, static_cast<u32>(m_Writes.size()), m_Writes.data(), 0, nullptr);
}

} // namespace VKit