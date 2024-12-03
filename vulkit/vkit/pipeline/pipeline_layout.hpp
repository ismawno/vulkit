#pragma once

#include "vkit/core/alias.hpp"
#include "vkit/backend/logical_device.hpp"

namespace VKit
{
class PipelineLayout
{
  public:
    class Builder
    {
      public:
        explicit Builder(const LogicalDevice::Proxy &p_Device) noexcept;

        Result<PipelineLayout> Build() const noexcept;

        Builder &AddDescriptorSetLayout(VkDescriptorSetLayout p_Layout) noexcept;
        Builder &AddPushConstantRange(VkShaderStageFlags p_Stages, u32 p_Size, u32 p_Offset = 0) noexcept;
        template <typename T> Builder &AddPushConstantRange(VkShaderStageFlags p_Stages, u32 p_Offset = 0) noexcept
        {
            return AddPushConstantRange(p_Stages, sizeof(T), p_Offset);
        }

        Builder &SetFlags(VkPipelineLayoutCreateFlags p_Flags) noexcept;
        Builder &AddFlags(VkPipelineLayoutCreateFlags p_Flags) noexcept;
        Builder &RemoveFlags(VkPipelineLayoutCreateFlags p_Flags) noexcept;

      private:
        LogicalDevice::Proxy m_Device;

        DynamicArray<VkDescriptorSetLayout> m_DescriptorSetLayouts;
        DynamicArray<VkPushConstantRange> m_PushConstantRanges;
        VkPipelineLayoutCreateFlags m_Flags = 0;
    };

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) noexcept;

    VkPipelineLayout GetLayout() const noexcept;
    explicit(false) operator VkPipelineLayout() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    PipelineLayout(const LogicalDevice::Proxy &p_Device, VkPipelineLayout p_Layout) noexcept;

    LogicalDevice::Proxy m_Device;
    VkPipelineLayout m_Layout;
};
} // namespace VKit