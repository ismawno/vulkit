#include "vkit/core/pch.hpp"
#include "vkit/descriptors/descriptor_set_layout.hpp"

namespace VKit
{
DescriptorSetLayout::Builder::Builder(const LogicalDevice::Proxy &p_Device) : m_Device(p_Device)
{
}

Result<DescriptorSetLayout> DescriptorSetLayout::Builder::Build() const
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCreateDescriptorSetLayout, Result<DescriptorSetLayout>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkDestroyDescriptorSetLayout, Result<DescriptorSetLayout>);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = m_Bindings.GetSize();
    layoutInfo.pBindings = m_Bindings.GetData();

    VkDescriptorSetLayout layout;
    const VkResult result =
        m_Device.Table->CreateDescriptorSetLayout(m_Device, &layoutInfo, m_Device.AllocationCallbacks, &layout);
    if (result != VK_SUCCESS)
        return Result<DescriptorSetLayout>::Error(result, "Failed to create descriptor set layout");
    return Result<DescriptorSetLayout>::Ok(m_Device, layout, m_Bindings);
}

DescriptorSetLayout::DescriptorSetLayout(const LogicalDevice::Proxy &p_Device, const VkDescriptorSetLayout p_Layout,
                                         const TKit::StaticArray16<VkDescriptorSetLayoutBinding> &p_Bindings)
    : m_Device(p_Device), m_Layout(p_Layout), m_Bindings{p_Bindings}
{
}

void DescriptorSetLayout::Destroy()
{
    TKIT_ASSERT(m_Layout, "[VULKIT] The descriptor set layout is a NULL handle");
    m_Device.Table->DestroyDescriptorSetLayout(m_Device, m_Layout, m_Device.AllocationCallbacks);
    m_Layout = VK_NULL_HANDLE;
}
void DescriptorSetLayout::SubmitForDeletion(DeletionQueue &p_Queue) const
{
    const VkDescriptorSetLayout layout = m_Layout;
    const LogicalDevice::Proxy device = m_Device;
    p_Queue.Push([=] { device.Table->DestroyDescriptorSetLayout(device, layout, device.AllocationCallbacks); });
}

const TKit::StaticArray16<VkDescriptorSetLayoutBinding> &DescriptorSetLayout::GetBindings() const
{
    return m_Bindings;
}
const LogicalDevice::Proxy &DescriptorSetLayout::GetDevice() const
{
    return m_Device;
}
VkDescriptorSetLayout DescriptorSetLayout::GetHandle() const
{
    return m_Layout;
}
DescriptorSetLayout::operator VkDescriptorSetLayout() const
{
    return m_Layout;
}
DescriptorSetLayout::operator bool() const
{
    return m_Layout != VK_NULL_HANDLE;
}

DescriptorSetLayout::Builder &DescriptorSetLayout::Builder::AddBinding(VkDescriptorType p_Type,
                                                                       VkShaderStageFlags p_StageFlags, u32 p_Count)
{
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = m_Bindings.GetSize();
    binding.descriptorType = p_Type;
    binding.descriptorCount = p_Count;
    binding.stageFlags = p_StageFlags;
    m_Bindings.Append(binding);
    return *this;
}

} // namespace VKit
