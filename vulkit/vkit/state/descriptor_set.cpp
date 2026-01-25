#include "vkit/core/pch.hpp"
#include "vkit/state/descriptor_set.hpp"
#include "vkit/resource/device_buffer.hpp"

namespace VKit
{
void DescriptorSet::Bind(const VkCommandBuffer commandBuffer, const VkPipelineBindPoint bindPoint,
                         const VkPipelineLayout layout, const TKit::Span<const u32> dynamicOffsets) const
{
    Bind(m_Device, commandBuffer, m_Set, bindPoint, layout, 0, dynamicOffsets);
}
void DescriptorSet::Bind(const ProxyDevice &device, const VkCommandBuffer commandBuffer,
                         const TKit::Span<const VkDescriptorSet> sets, const VkPipelineBindPoint bindPoint,
                         const VkPipelineLayout layout, const u32 firstSet, const TKit::Span<const u32> dynamicOffsets)
{
    device.Table->CmdBindDescriptorSets(commandBuffer, bindPoint, layout, firstSet, sets.GetSize(), sets.GetData(),
                                        dynamicOffsets.GetSize(), dynamicOffsets.GetData());
}

void DescriptorSet::Writer::WriteBuffer(const u32 binding, const VkDescriptorBufferInfo &bufferInfo)
{
    const VkDescriptorSetLayoutBinding &description = m_Layout->GetBindings()[binding];

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = description.descriptorType;
    write.dstBinding = binding;
    write.pBufferInfo = &bufferInfo;

    // I am not sure if this is correct!!
    write.descriptorCount = description.descriptorCount;

    m_Writes.Append(write);
}

void DescriptorSet::Writer::WriteBuffer(const u32 binding, const DeviceBuffer &buffer)
{
    WriteBuffer(binding, buffer.CreateDescriptorInfo());
}

void DescriptorSet::Writer::WriteImage(const u32 binding, const VkDescriptorImageInfo &imageInfo)
{
    const VkDescriptorSetLayoutBinding &description = m_Layout->GetBindings()[binding];

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = description.descriptorType;
    write.dstBinding = binding;
    write.pImageInfo = &imageInfo;

    // I am not sure if this is correct!!
    write.descriptorCount = description.descriptorCount;

    m_Writes.Append(write);
}

void DescriptorSet::Writer::Overwrite(const VkDescriptorSet set)
{
    for (VkWriteDescriptorSet &write : m_Writes)
        write.dstSet = set;
    m_Device.Table->UpdateDescriptorSets(m_Device, m_Writes.GetSize(), m_Writes.GetData(), 0, nullptr);
}

} // namespace VKit
