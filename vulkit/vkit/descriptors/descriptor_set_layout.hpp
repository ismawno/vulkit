#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/backend/logical_device.hpp"
#include "tkit/core/non_copyable.hpp"

#include <vulkan/vulkan.hpp>
#include <span>

namespace VKit
{
class VKIT_API DescriptorSetLayout
{
  public:
    class Builder
    {
      public:
        explicit Builder(const LogicalDevice *p_Device) noexcept;

        Result<DescriptorSetLayout> Build() const noexcept;

        Builder &AddBinding(VkDescriptorType p_Type, VkShaderStageFlags p_StageFlags, u32 p_Count = 1) noexcept;

      private:
        const LogicalDevice *m_Device;

        DynamicArray<VkDescriptorSetLayoutBinding> m_Bindings;
    };

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) noexcept;

    VkDescriptorSetLayout GetLayout() const noexcept;
    const DynamicArray<VkDescriptorSetLayoutBinding> &GetBindings() const noexcept;

  private:
    DescriptorSetLayout(const LogicalDevice &p_Device, VkDescriptorSetLayout p_Layout,
                        const DynamicArray<VkDescriptorSetLayoutBinding> &p_Bindings) noexcept;

    LogicalDevice m_Device;
    VkDescriptorSetLayout m_Layout;
    DynamicArray<VkDescriptorSetLayoutBinding> m_Bindings;
};
} // namespace VKit