#include "vkit/core/pch.hpp"
#include "vkit/state/pipeline_layout.hpp"

namespace VKit
{

Result<PipelineLayout> PipelineLayout::Builder::Build() const
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCreatePipelineLayout, Result<PipelineLayout>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkDestroyPipelineLayout, Result<PipelineLayout>);

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
        return Result<PipelineLayout>::Error(result, "Failed to create pipeline layout");

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

PipelineLayout::Builder &PipelineLayout::Builder::AddDescriptorSetLayout(const VkDescriptorSetLayout p_Layout)
{
    m_DescriptorSetLayouts.Append(p_Layout);
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::AddPushConstantRange(const VkShaderStageFlags p_Stages,
                                                                       const u32 p_Size, const u32 p_Offset)
{
    m_PushConstantRanges.Append(VkPushConstantRange{p_Stages, p_Offset, p_Size});
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::SetFlags(const VkPipelineLayoutCreateFlags p_Flags)
{
    m_Flags = p_Flags;
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::AddFlags(const VkPipelineLayoutCreateFlags p_Flags)
{
    m_Flags |= p_Flags;
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::RemoveFlags(const VkPipelineLayoutCreateFlags p_Flags)
{
    m_Flags &= ~p_Flags;
    return *this;
}

} // namespace VKit
