#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/core/device.hpp"
#include "tkit/core/non_copyable.hpp"

#include <vulkan/vulkan.hpp>
#include <span>

namespace VKit
{
class VKIT_API DescriptorSetLayout : public TKit::RefCounted<DescriptorSetLayout>
{
    TKIT_NON_COPYABLE(DescriptorSetLayout)
  public:
    DescriptorSetLayout(std::span<const VkDescriptorSetLayoutBinding> p_Bindings) noexcept;
    ~DescriptorSetLayout() noexcept;

    static VkDescriptorSetLayoutBinding CreateBinding(u32 p_Binding, VkDescriptorType p_DescriptorType,
                                                      VkShaderStageFlags p_StageFlags, u32 p_Count = 1) noexcept;

    const VkDescriptorSetLayoutBinding &GetBinding(usize p_Index) const noexcept;
    VkDescriptorSetLayout GetLayout() const noexcept;

    usize GetBindingCount() const noexcept;

  private:
    TKit::Ref<Device> m_Device;
    VkDescriptorSetLayout m_Layout;
    DynamicArray<VkDescriptorSetLayoutBinding> m_Bindings;
};
} // namespace VKit