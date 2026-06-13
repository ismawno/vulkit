#include "vkit/core/pch.hpp"
#include "vkit/state/descriptor_set_layout.hpp"

namespace VKit
{

Result<DescriptorSetLayout> DescriptorSetLayout::Builder::Build() const
{
    TKit::StackArray<VkDescriptorSetLayoutBinding> bindings{};
    bindings.Reserve(m_Bindings.GetSize());

    for (const auto &kv : m_Bindings)
        bindings.Append(kv.Value);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.GetSize();
    layoutInfo.pBindings = bindings.GetData();

#if defined(VKIT_API_VERSION_1_2) || defined(VK_EXT_descriptor_indexing)
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT flagsInfo{};
    if (!m_BindFlags.IsEmpty())
    {
        flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
        flagsInfo.bindingCount = m_BindFlags.GetSize();
        flagsInfo.pBindingFlags = m_BindFlags.GetData();
        layoutInfo.pNext = &flagsInfo;
    }
#endif
    layoutInfo.flags = m_Flags;

    VkDescriptorSetLayout layout;
    VKIT_RETURN_IF_FAILED(
        m_Device.Table->CreateDescriptorSetLayout(m_Device, &layoutInfo, m_Device.AllocationCallbacks, &layout),
        Result<DescriptorSetLayout>);

    return Result<DescriptorSetLayout>::Ok(m_Device, layout, m_Bindings);
}

void DescriptorSetLayout::Destroy()
{
    if (m_Layout)
    {
        m_Device.Table->DestroyDescriptorSetLayout(m_Device, m_Layout, m_Device.AllocationCallbacks);
        m_Layout = VK_NULL_HANDLE;
    }
}
DescriptorSetLayout::Builder &DescriptorSetLayout::Builder::SetFlags(const VkDescriptorSetLayoutCreateFlags flags)
{
    m_Flags = flags;
    return *this;
}
#if defined(VKIT_API_VERSION_1_2) || defined(VK_EXT_descriptor_indexing)
DescriptorSetLayout::Builder &DescriptorSetLayout::Builder::AddBinding2(const u32 binding, const VkDescriptorType type,
                                                                        const VkShaderStageFlags stageFlags,
                                                                        const u32 count,
                                                                        const VkDescriptorBindingFlagsEXT flags)
{
    m_BindFlags.Append(flags);
    return AddBinding(binding, type, stageFlags, count);
}
#endif
DescriptorSetLayout::Builder &DescriptorSetLayout::Builder::AddBinding(const u32 bpoint, const VkDescriptorType type,
                                                                       const VkShaderStageFlags stageFlags,
                                                                       const u32 count)
{
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = bpoint;
    binding.descriptorType = type;
    binding.descriptorCount = count;
    binding.stageFlags = stageFlags;
    m_Bindings[bpoint] = binding;
    return *this;
}

} // namespace VKit
