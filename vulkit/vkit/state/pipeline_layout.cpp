#include "vkit/core/pch.hpp"
#include "vkit/state/pipeline_layout.hpp"

namespace VKit
{

Result<PipelineLayout> PipelineLayout::Builder::Build() const
{
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = m_DescriptorSetLayouts.GetSize();
    layoutInfo.pSetLayouts = m_DescriptorSetLayouts.IsEmpty() ? nullptr : m_DescriptorSetLayouts.GetData();
    layoutInfo.pushConstantRangeCount = m_PushConstantRanges.GetSize();
    layoutInfo.pPushConstantRanges = m_PushConstantRanges.IsEmpty() ? nullptr : m_PushConstantRanges.GetData();
    layoutInfo.flags = m_Flags;

    VkPipelineLayout layout;
    const VkResult result =
        m_Device.Table->CreatePipelineLayout(m_Device, &layoutInfo, m_Device.AllocationCallbacks, &layout);
    if (result != VK_SUCCESS)
        return Result<PipelineLayout>::Error(result);

    PipelineLayout::Info info;
    info.DescriptorSetLayouts = m_DescriptorSetLayouts;
    info.PushConstantRanges = m_PushConstantRanges;
    return Result<PipelineLayout>::Ok(m_Device, layout, info);
}

void PipelineLayout::Destroy()
{
    if (m_Layout)
    {
        m_Device.Table->DestroyPipelineLayout(m_Device, m_Layout, m_Device.AllocationCallbacks);
        m_Layout = VK_NULL_HANDLE;
    }
}

PipelineLayout::Builder &PipelineLayout::Builder::AddDescriptorSetLayout(const VkDescriptorSetLayout layout)
{
    m_DescriptorSetLayouts.Append(layout);
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::AddPushConstantRange(const VkShaderStageFlags stages, const u32 size,
                                                                       const u32 offset)
{
    m_PushConstantRanges.Append(VkPushConstantRange{stages, offset, size});
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::SetFlags(const VkPipelineLayoutCreateFlags flags)
{
    m_Flags = flags;
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::AddFlags(const VkPipelineLayoutCreateFlags flags)
{
    m_Flags |= flags;
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::RemoveFlags(const VkPipelineLayoutCreateFlags flags)
{
    m_Flags &= ~flags;
    return *this;
}

} // namespace VKit
