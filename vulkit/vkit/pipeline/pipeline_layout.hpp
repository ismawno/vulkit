#pragma once

#ifndef VKIT_ENABLE_PIPELINE_LAYOUT
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_PIPELINE_LAYOUT"
#endif

#include "vkit/vulkan/logical_device.hpp"

namespace VKit
{
class VKIT_API PipelineLayout
{
  public:
    class Builder
    {
      public:
        Builder(const LogicalDevice::Proxy &p_Device) : m_Device(p_Device)
        {
        }

        Result<PipelineLayout> Build() const;

        Builder &AddDescriptorSetLayout(VkDescriptorSetLayout p_Layout);
        Builder &AddPushConstantRange(VkShaderStageFlags p_Stages, u32 p_Size, u32 p_Offset = 0);
        template <typename T> Builder &AddPushConstantRange(VkShaderStageFlags p_Stages, u32 p_Offset = 0)
        {
            return AddPushConstantRange(p_Stages, sizeof(T), p_Offset);
        }

        Builder &SetFlags(VkPipelineLayoutCreateFlags p_Flags);
        Builder &AddFlags(VkPipelineLayoutCreateFlags p_Flags);
        Builder &RemoveFlags(VkPipelineLayoutCreateFlags p_Flags);

      private:
        LogicalDevice::Proxy m_Device;

        TKit::Array8<VkDescriptorSetLayout> m_DescriptorSetLayouts;
        TKit::Array4<VkPushConstantRange> m_PushConstantRanges;
        VkPipelineLayoutCreateFlags m_Flags = 0;
    };

    struct Info
    {
        TKit::Array8<VkDescriptorSetLayout> DescriptorSetLayouts;
        TKit::Array4<VkPushConstantRange> PushConstantRanges;
    };

    PipelineLayout() = default;
    PipelineLayout(const LogicalDevice::Proxy &p_Device, const VkPipelineLayout p_Layout, const Info &p_Info)
        : m_Device(p_Device), m_Layout(p_Layout), m_Info(p_Info)
    {
    }

    void Destroy();

    const Info &GetInfo() const
    {
        return m_Info;
    }

    const LogicalDevice::Proxy &GetDevice() const
    {
        return m_Device;
    }
    VkPipelineLayout GetHandle() const
    {
        return m_Layout;
    }
    operator VkPipelineLayout() const
    {
        return m_Layout;
    }
    operator bool() const
    {
        return m_Layout != VK_NULL_HANDLE;
    }

  private:
    LogicalDevice::Proxy m_Device{};
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
