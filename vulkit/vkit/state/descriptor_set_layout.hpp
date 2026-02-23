#pragma once

#ifndef VKIT_ENABLE_DESCRIPTORS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_DESCRIPTORS"
#endif

#include "vkit/device/proxy_device.hpp"
#include <vulkan/vulkan.h>

namespace VKit
{
class DescriptorSetLayout
{
  public:
    class Builder
    {
      public:
        Builder(const ProxyDevice &device) : m_Device(device)
        {
        }

        VKIT_NO_DISCARD Result<DescriptorSetLayout> Build() const;

        Builder &AddBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, u32 count = 1);

      private:
        ProxyDevice m_Device;
        TKit::TierArray<VkDescriptorSetLayoutBinding> m_Bindings;
    };

    DescriptorSetLayout() = default;
    DescriptorSetLayout(const ProxyDevice &device, const VkDescriptorSetLayout layout,
                        const TKit::TierArray<VkDescriptorSetLayoutBinding> &bindings)
        : m_Device(device), m_Layout(layout), m_Bindings{bindings}
    {
    }

    void Destroy();
    VKIT_SET_DEBUG_NAME(m_Layout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)

    const TKit::TierArray<VkDescriptorSetLayoutBinding> &GetBindings() const
    {
        return m_Bindings;
    }
    const ProxyDevice &GetDevice() const
    {
        return m_Device;
    }
    VkDescriptorSetLayout GetHandle() const
    {
        return m_Layout;
    }
    operator VkDescriptorSetLayout() const
    {
        return m_Layout;
    }
    operator bool() const
    {
        return m_Layout != VK_NULL_HANDLE;
    }

  private:
    ProxyDevice m_Device{};
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    TKit::TierArray<VkDescriptorSetLayoutBinding> m_Bindings;
};
} // namespace VKit
