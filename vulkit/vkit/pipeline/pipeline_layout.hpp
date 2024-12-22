#pragma once

#include "vkit/core/alias.hpp"
#include "vkit/backend/logical_device.hpp"

namespace VKit
{
/**
 * @brief Represents a Vulkan pipeline layout.
 *
 * Defines the layout for a pipeline, including descriptor set layouts and push constant ranges.
 * Used by Vulkan pipelines to manage resource bindings and shader constants.
 */
class VKIT_API PipelineLayout
{
  public:
    /**
     * @brief A utility for creating and configuring a Vulkan pipeline layout.
     *
     * Provides methods to define descriptor set layouts, push constant ranges, and layout creation flags.
     * Simplifies the process of setting up pipeline layouts for Vulkan applications.
     */
    class Builder
    {
      public:
        explicit Builder(const LogicalDevice::Proxy &p_Device) noexcept;

        /**
         * @brief Creates a pipeline layout based on the builder's configuration.
         *
         * Returns a pipeline layout object if the creation succeeds, or an error otherwise.
         *
         * @return A result containing the created PipelineLayout or an error.
         */
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

        TKit::StaticArray8<VkDescriptorSetLayout> m_DescriptorSetLayouts;
        TKit::StaticArray4<VkPushConstantRange> m_PushConstantRanges;
        VkPipelineLayoutCreateFlags m_Flags = 0;
    };

    PipelineLayout() noexcept = default;
    PipelineLayout(const LogicalDevice::Proxy &p_Device, VkPipelineLayout p_Layout,
                   const TKit::StaticArray4<VkPushConstantRange> &p_PushConstantRanges) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    const TKit::StaticArray4<VkPushConstantRange> &GetPushConstantRanges() const noexcept;

    VkPipelineLayout GetLayout() const noexcept;
    explicit(false) operator VkPipelineLayout() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    TKit::StaticArray4<VkPushConstantRange> m_PushConstantRanges;
};
} // namespace VKit