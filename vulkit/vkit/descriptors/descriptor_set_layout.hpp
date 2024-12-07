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
        explicit Builder(const LogicalDevice::Proxy &p_Device) noexcept;

        Result<DescriptorSetLayout> Build() const noexcept;

        Builder &AddBinding(VkDescriptorType p_Type, VkShaderStageFlags p_StageFlags, u32 p_Count = 1) noexcept;

      private:
        LogicalDevice::Proxy m_Device;

        DynamicArray<VkDescriptorSetLayoutBinding> m_Bindings;
    };

    DescriptorSetLayout() noexcept = default;
    DescriptorSetLayout(const LogicalDevice::Proxy &p_Device, VkDescriptorSetLayout p_Layout,
                        const DynamicArray<VkDescriptorSetLayoutBinding> &p_Bindings) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    VkDescriptorSetLayout GetLayout() const noexcept;
    explicit(false) operator VkDescriptorSetLayout() const noexcept;
    explicit(false) operator bool() const noexcept;

    const DynamicArray<VkDescriptorSetLayoutBinding> &GetBindings() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    DynamicArray<VkDescriptorSetLayoutBinding> m_Bindings;
};
} // namespace VKit