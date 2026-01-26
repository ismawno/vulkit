#pragma once

#ifndef VKIT_ENABLE_PIPELINE_LAYOUT
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_PIPELINE_LAYOUT"
#endif

#include "vkit/device/proxy_device.hpp"

namespace VKit
{
class PipelineLayout
{
  public:
    class Builder
    {
      public:
        Builder(const ProxyDevice &device) : m_Device(device)
        {
        }

        VKIT_NO_DISCARD Result<PipelineLayout> Build() const;

        Builder &AddDescriptorSetLayout(VkDescriptorSetLayout layout);
        Builder &AddPushConstantRange(VkShaderStageFlags stages, u32 size, u32 offset = 0);
        template <typename T> Builder &AddPushConstantRange(VkShaderStageFlags stages, u32 offset = 0)
        {
            return AddPushConstantRange(stages, sizeof(T), offset);
        }

        Builder &SetFlags(VkPipelineLayoutCreateFlags flags);
        Builder &AddFlags(VkPipelineLayoutCreateFlags flags);
        Builder &RemoveFlags(VkPipelineLayoutCreateFlags flags);

      private:
        ProxyDevice m_Device;

        TKit::TierArray<VkDescriptorSetLayout> m_DescriptorSetLayouts;
        TKit::TierArray<VkPushConstantRange> m_PushConstantRanges;
        VkPipelineLayoutCreateFlags m_Flags = 0;
    };

    struct Info
    {
        TKit::TierArray<VkDescriptorSetLayout> DescriptorSetLayouts;
        TKit::TierArray<VkPushConstantRange> PushConstantRanges;
    };

    PipelineLayout() = default;
    PipelineLayout(const ProxyDevice &device, const VkPipelineLayout layout, const Info &info)
        : m_Device(device), m_Layout(layout), m_Info(info)
    {
    }

    void Destroy();

    const Info &GetInfo() const
    {
        return m_Info;
    }

    const ProxyDevice &GetDevice() const
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
    ProxyDevice m_Device{};
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
