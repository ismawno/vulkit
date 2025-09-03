#include "vkit/core/pch.hpp"
#include "vkit/descriptors/descriptor_set.hpp"
#include "vkit/buffer/buffer.hpp"

namespace VKit
{
Result<DescriptorSet> DescriptorSet::Create(const LogicalDevice::Proxy &p_Device, const VkDescriptorSet p_Set)
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkUpdateDescriptorSets, Result<DescriptorSet>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCmdBindDescriptorSets, Result<DescriptorSet>);

    return Result<DescriptorSet>::Ok(p_Device, p_Set);
}

DescriptorSet::DescriptorSet(const LogicalDevice::Proxy &p_Device, const VkDescriptorSet p_Set)
    : m_Device(p_Device), m_Set(p_Set)
{
}

void DescriptorSet::Bind(const VkCommandBuffer p_CommandBuffer, const VkPipelineBindPoint p_BindPoint,
                         const VkPipelineLayout p_Layout, const TKit::Span<const u32> p_DynamicOffsets) const
{
    Bind(m_Device, p_CommandBuffer, m_Set, p_BindPoint, p_Layout, 0, p_DynamicOffsets);
}
void DescriptorSet::Bind(const LogicalDevice::Proxy &p_Device, const VkCommandBuffer p_CommandBuffer,
                         const TKit::Span<const VkDescriptorSet> p_Sets, const VkPipelineBindPoint p_BindPoint,
                         const VkPipelineLayout p_Layout, const u32 p_FirstSet,
                         const TKit::Span<const u32> p_DynamicOffsets)
{
    if (!p_DynamicOffsets.IsEmpty())
        p_Device.Table->CmdBindDescriptorSets(p_CommandBuffer, p_BindPoint, p_Layout, p_FirstSet, p_Sets.GetSize(),
                                              p_Sets.GetData(), p_DynamicOffsets.GetSize(), p_DynamicOffsets.GetData());
    else
        p_Device.Table->CmdBindDescriptorSets(p_CommandBuffer, p_BindPoint, p_Layout, p_FirstSet, p_Sets.GetSize(),
                                              p_Sets.GetData(), 0, nullptr);
}
void DescriptorSet::Bind(const LogicalDevice::Proxy &p_Device, const VkCommandBuffer p_CommandBuffer,
                         const VkDescriptorSet p_Set, const VkPipelineBindPoint p_BindPoint,
                         const VkPipelineLayout p_Layout, const u32 p_FirstSet,
                         const TKit::Span<const u32> p_DynamicOffsets)
{
    if (!p_DynamicOffsets.IsEmpty())
        p_Device.Table->CmdBindDescriptorSets(p_CommandBuffer, p_BindPoint, p_Layout, p_FirstSet, 1, &p_Set,
                                              p_DynamicOffsets.GetSize(), p_DynamicOffsets.GetData());
    else
        p_Device.Table->CmdBindDescriptorSets(p_CommandBuffer, p_BindPoint, p_Layout, p_FirstSet, 1, &p_Set, 0,
                                              nullptr);
}

const LogicalDevice::Proxy &DescriptorSet::GetDevice() const
{
    return m_Device;
}
VkDescriptorSet DescriptorSet::GetHandle() const
{
    return m_Set;
}
DescriptorSet::operator VkDescriptorSet() const
{
    return m_Set;
}
DescriptorSet::operator bool() const
{
    return m_Set != VK_NULL_HANDLE;
}

DescriptorSet::Writer::Writer(const LogicalDevice::Proxy &p_Device, const DescriptorSetLayout *p_Layout)
    : m_Device(p_Device), m_Layout(p_Layout)
{
}

void DescriptorSet::Writer::WriteBuffer(const u32 p_Binding, const VkDescriptorBufferInfo &p_BufferInfo)
{
    const VkDescriptorSetLayoutBinding &description = m_Layout->GetBindings()[p_Binding];

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = description.descriptorType;
    write.dstBinding = p_Binding;
    write.pBufferInfo = &p_BufferInfo;

    // I am not sure if this is correct!!
    write.descriptorCount = description.descriptorCount;

    m_Writes.Append(write);
}

void DescriptorSet::Writer::WriteBuffer(const u32 p_Binding, const Buffer &p_Buffer)
{
    WriteBuffer(p_Binding, p_Buffer.GetDescriptorInfo());
}

void DescriptorSet::Writer::WriteImage(const u32 p_Binding, const VkDescriptorImageInfo &p_ImageInfo)
{
    const VkDescriptorSetLayoutBinding &description = m_Layout->GetBindings()[p_Binding];

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = description.descriptorType;
    write.dstBinding = p_Binding;
    write.pImageInfo = &p_ImageInfo;

    // I am not sure if this is correct!!
    write.descriptorCount = description.descriptorCount;

    m_Writes.Append(write);
}

void DescriptorSet::Writer::Overwrite(const VkDescriptorSet p_Set)
{
    for (VkWriteDescriptorSet &write : m_Writes)
        write.dstSet = p_Set;
    m_Device.Table->UpdateDescriptorSets(m_Device, m_Writes.GetSize(), m_Writes.GetData(), 0, nullptr);
}

} // namespace VKit