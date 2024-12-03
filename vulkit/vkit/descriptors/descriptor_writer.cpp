#include "vkit/core/pch.hpp"
#include "vkit/descriptors/descriptor_writer.hpp"

namespace VKit
{
DescriptorWriter::DescriptorWriter(const LogicalDevice *p_Device, const DescriptorSetLayout *p_Layout,
                                   const DescriptorPool *p_Pool) noexcept
    : m_Device(p_Device), m_Layout(p_Layout), m_Pool(p_Pool)
{
    m_Writes.reserve(m_Layout->GetBindings().size());
}

void DescriptorWriter::WriteBuffer(const u32 p_Binding, const VkDescriptorBufferInfo *p_BufferInfo) noexcept
{
    const VkDescriptorSetLayoutBinding &description = m_Layout->GetBindings()[p_Binding];

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = description.descriptorType;
    write.dstBinding = p_Binding;
    write.pBufferInfo = p_BufferInfo;

    // I am not sure if this is correct!!
    write.descriptorCount = description.descriptorCount;

    m_Writes.push_back(write);
}

void DescriptorWriter::WriteImage(const u32 p_Binding, const VkDescriptorImageInfo *p_ImageInfo) noexcept
{
    const VkDescriptorSetLayoutBinding &description = m_Layout->GetBindings()[p_Binding];

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = description.descriptorType;
    write.dstBinding = p_Binding;
    write.pImageInfo = p_ImageInfo;

    // I am not sure if this is correct!!
    write.descriptorCount = description.descriptorCount;

    m_Writes.push_back(write);
}

Result<VkDescriptorSet> DescriptorWriter::Build() noexcept
{
    const auto result = m_Pool->Allocate(m_Layout->GetLayout());
    if (!result)
        return result;
    Overwrite(result.GetValue());
    return result;
}

void DescriptorWriter::Overwrite(const VkDescriptorSet p_Set) noexcept
{
    for (VkWriteDescriptorSet &write : m_Writes)
        write.dstSet = p_Set;
    vkUpdateDescriptorSets(m_Device->GetDevice(), static_cast<u32>(m_Writes.size()), m_Writes.data(), 0, nullptr);
}

} // namespace VKit