#include "vkit/core/pch.hpp"
#include "vkit/descriptors/descriptor_set_layout.hpp"

namespace VKit
{
DescriptorSetLayout::Builder::Builder(const LogicalDevice *p_Device) noexcept : m_Device(p_Device)
{
}

Result<DescriptorSetLayout> DescriptorSetLayout::Builder::Build() const noexcept
{
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<u32>(m_Bindings.size());
    layoutInfo.pBindings = m_Bindings.data();

    VkDescriptorSetLayout layout;
    const VkResult result = vkCreateDescriptorSetLayout(m_Device->GetDevice(), &layoutInfo,
                                                        m_Device->GetInstance().GetInfo().AllocationCallbacks, &layout);
    if (result != VK_SUCCESS)
        return Result<DescriptorSetLayout>::Error(result, "Failed to create descriptor set layout");
    return Result<DescriptorSetLayout>::Ok(*m_Device, layout, m_Bindings);
}

DescriptorSetLayout::DescriptorSetLayout(const LogicalDevice &p_Device, const VkDescriptorSetLayout p_Layout,
                                         const DynamicArray<VkDescriptorSetLayoutBinding> &p_Bindings) noexcept
    : m_Device(p_Device), m_Layout(p_Layout), m_Bindings{p_Bindings}
{
}

void DescriptorSetLayout::Destroy() noexcept
{
    vkDestroyDescriptorSetLayout(m_Device.GetDevice(), m_Layout, m_Device.GetInstance().GetInfo().AllocationCallbacks);
}
void DescriptorSetLayout::SubmitForDeletion(DeletionQueue &p_Queue) noexcept
{
    const VkDescriptorSetLayout layout = m_Layout;
    const VkDevice device = m_Device.GetDevice();
    const VkAllocationCallbacks *alloc = m_Device.GetInstance().GetInfo().AllocationCallbacks;
    p_Queue.Push([layout, device, alloc]() { vkDestroyDescriptorSetLayout(device, layout, alloc); });
}

const DynamicArray<VkDescriptorSetLayoutBinding> &DescriptorSetLayout::GetBindings() const noexcept
{
    return m_Bindings;
}
VkDescriptorSetLayout DescriptorSetLayout::GetLayout() const noexcept
{
    return m_Layout;
}

DescriptorSetLayout::Builder &DescriptorSetLayout::Builder::AddBinding(VkDescriptorType p_Type,
                                                                       VkShaderStageFlags p_StageFlags,
                                                                       u32 p_Count) noexcept
{
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = static_cast<u32>(m_Bindings.size());
    binding.descriptorType = p_Type;
    binding.descriptorCount = p_Count;
    binding.stageFlags = p_StageFlags;
    m_Bindings.push_back(binding);
    return *this;
}

} // namespace VKit