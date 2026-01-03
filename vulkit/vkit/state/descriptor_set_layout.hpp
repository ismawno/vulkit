#pragma once

#ifndef VKIT_ENABLE_DESCRIPTORS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_DESCRIPTORS"
#endif

#include "vkit/device/logical_device.hpp"
#include <vulkan/vulkan.h>

namespace VKit
{
class DescriptorSetLayout
{
  public:
    class Builder
    {
      public:
        Builder(const ProxyDevice &p_Device) : m_Device(p_Device)
        {
        }

        Result<DescriptorSetLayout> Build() const;

        Builder &AddBinding(VkDescriptorType p_Type, VkShaderStageFlags p_StageFlags, u32 p_Count = 1);

      private:
        ProxyDevice m_Device;
        TKit::Array16<VkDescriptorSetLayoutBinding> m_Bindings;
    };

    DescriptorSetLayout() = default;
    DescriptorSetLayout(const ProxyDevice &p_Device, const VkDescriptorSetLayout p_Layout,
                        const TKit::Array16<VkDescriptorSetLayoutBinding> &p_Bindings)
        : m_Device(p_Device), m_Layout(p_Layout), m_Bindings{p_Bindings}
    {
    }

    void Destroy();

    const TKit::Array16<VkDescriptorSetLayoutBinding> &GetBindings() const
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
    TKit::Array16<VkDescriptorSetLayoutBinding> m_Bindings;
};
} // namespace VKit
