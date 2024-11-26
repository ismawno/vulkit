#include "vkit/core/pch.hpp"
#include "vkit/descriptors/descriptor_set_layout.hpp"
#include "vkit/core/core.hpp"

namespace VKit
{
DescriptorSetLayout::DescriptorSetLayout(const std::span<const VkDescriptorSetLayoutBinding> p_Bindings) noexcept
    : m_Bindings{p_Bindings.begin(), p_Bindings.end()}
{
    m_Device = Core::GetDevice();
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<u32>(m_Bindings.size());
    layoutInfo.pBindings = m_Bindings.data();

    TKIT_ASSERT_RETURNS(vkCreateDescriptorSetLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &m_Layout), VK_SUCCESS,
                        "Failed to create descriptor set layout");
}

DescriptorSetLayout::~DescriptorSetLayout() noexcept
{
    vkDestroyDescriptorSetLayout(m_Device->GetDevice(), m_Layout, nullptr);
}

VkDescriptorSetLayoutBinding DescriptorSetLayout::CreateBinding(const u32 p_Binding,
                                                                const VkDescriptorType p_DescriptorType,
                                                                const VkShaderStageFlags p_StageFlags,
                                                                const u32 p_Count) noexcept
{
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = p_Binding;
    binding.descriptorType = p_DescriptorType;
    binding.descriptorCount = p_Count;
    binding.stageFlags = p_StageFlags;
    return binding;
}

const VkDescriptorSetLayoutBinding &DescriptorSetLayout::GetBinding(const usize p_Index) const noexcept
{
    return m_Bindings[p_Index];
}

VkDescriptorSetLayout DescriptorSetLayout::GetLayout() const noexcept
{
    return m_Layout;
}

usize DescriptorSetLayout::GetBindingCount() const noexcept
{
    return m_Bindings.size();
}

} // namespace VKit