#include "vkit/core/pch.hpp"
#include "vkit/pipeline/pipeline_layout.hpp"

namespace VKit
{
PipelineLayout::Builder::Builder(const LogicalDevice::Proxy &p_Device) noexcept : m_Device(p_Device)
{
}

Result<PipelineLayout> PipelineLayout::Builder::Build() const noexcept
{
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = m_DescriptorSetLayouts.size();
    layoutInfo.pSetLayouts = m_DescriptorSetLayouts.empty() ? nullptr : m_DescriptorSetLayouts.data();
    layoutInfo.pushConstantRangeCount = m_PushConstantRanges.size();
    layoutInfo.pPushConstantRanges = m_PushConstantRanges.empty() ? nullptr : m_PushConstantRanges.data();
    layoutInfo.flags = m_Flags;

    VkPipelineLayout layout;
    const VkResult result = vkCreatePipelineLayout(m_Device, &layoutInfo, m_Device.AllocationCallbacks, &layout);
    if (result != VK_SUCCESS)
        return Result<PipelineLayout>::Error(result, "Failed to create pipeline layout");

    PipelineLayout::Info info;
    info.DescriptorSetLayouts = m_DescriptorSetLayouts;
    info.PushConstantRanges = m_PushConstantRanges;
    return Result<PipelineLayout>::Ok(m_Device, layout, info);
}

PipelineLayout::PipelineLayout(const LogicalDevice::Proxy &p_Device, const VkPipelineLayout p_Layout,
                               const Info &p_Info) noexcept
    : m_Device(p_Device), m_Layout(p_Layout), m_Info(p_Info)
{
}

void PipelineLayout::Destroy() noexcept
{
    TKIT_ASSERT(m_Layout, "[VULKIT] The pipeline layout is a NULL handle");
    vkDestroyPipelineLayout(m_Device, m_Layout, m_Device.AllocationCallbacks);
    m_Layout = VK_NULL_HANDLE;
}

void PipelineLayout::SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
{
    const VkPipelineLayout layout = m_Layout;
    const LogicalDevice::Proxy device = m_Device;
    p_Queue.Push([layout, device]() { vkDestroyPipelineLayout(device, layout, device.AllocationCallbacks); });
}
const PipelineLayout::Info &PipelineLayout::GetInfo() const noexcept
{
    return m_Info;
}
VkPipelineLayout PipelineLayout::GetLayout() const noexcept
{
    return m_Layout;
}
PipelineLayout::operator VkPipelineLayout() const noexcept
{
    return m_Layout;
}
PipelineLayout::operator bool() const noexcept
{
    return m_Layout != VK_NULL_HANDLE;
}

PipelineLayout::Builder &PipelineLayout::Builder::AddDescriptorSetLayout(const VkDescriptorSetLayout p_Layout) noexcept
{
    m_DescriptorSetLayouts.push_back(p_Layout);
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::AddPushConstantRange(const VkShaderStageFlags p_Stages,
                                                                       const u32 p_Size, const u32 p_Offset) noexcept
{
    m_PushConstantRanges.push_back(VkPushConstantRange{p_Stages, p_Offset, p_Size});
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::SetFlags(const VkPipelineLayoutCreateFlags p_Flags) noexcept
{
    m_Flags = p_Flags;
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::AddFlags(const VkPipelineLayoutCreateFlags p_Flags) noexcept
{
    m_Flags |= p_Flags;
    return *this;
}
PipelineLayout::Builder &PipelineLayout::Builder::RemoveFlags(const VkPipelineLayoutCreateFlags p_Flags) noexcept
{
    m_Flags &= ~p_Flags;
    return *this;
}

} // namespace VKit