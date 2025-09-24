#pragma once

#ifndef VKIT_ENABLE_PIPELINE_LAYOUT
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_PIPELINE_LAYOUT"
#endif

#include "vkit/vulkan/logical_device.hpp"

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
        Builder(const LogicalDevice::Proxy &p_Device) : m_Device(p_Device)
        {
        }

        /**
         * @brief Creates a pipeline layout based on the builder's configuration.
         *
         * Returns a pipeline layout object if the creation succeeds, or an error otherwise.
         *
         * @return A `Result` containing the created `PipelineLayout` or an error.
         */
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

        TKit::StaticArray8<VkDescriptorSetLayout> m_DescriptorSetLayouts;
        TKit::StaticArray4<VkPushConstantRange> m_PushConstantRanges;
        VkPipelineLayoutCreateFlags m_Flags = 0;
    };

    struct Info
    {
        TKit::StaticArray8<VkDescriptorSetLayout> DescriptorSetLayouts;
        TKit::StaticArray4<VkPushConstantRange> PushConstantRanges;
    };

    PipelineLayout() = default;
    PipelineLayout(const LogicalDevice::Proxy &p_Device, const VkPipelineLayout p_Layout, const Info &p_Info)
        : m_Device(p_Device), m_Layout(p_Layout), m_Info(p_Info)
    {
    }

    void Destroy();
    void SubmitForDeletion(DeletionQueue &p_Queue) const;

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
