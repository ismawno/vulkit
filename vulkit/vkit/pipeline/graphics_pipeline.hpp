#pragma once

#ifndef VKIT_ENABLE_GRAPHICS_PIPELINE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_GRAPHICS_PIPELINE"
#endif

#include "vkit/vulkan/logical_device.hpp"
#include <vulkan/vulkan.h>

namespace VKit
{
/**
 * @brief Represents a Vulkan graphics pipeline.
 *
 * Handles the creation, management, and binding of graphics pipelines, which are used
 * for rendering in Vulkan. Includes functionality for custom specifications and batch creation.
 */
class VKIT_API GraphicsPipeline
{
  public:
    class Builder;

  private:
    class ColorAttachmentBuilder
    {
      public:
        ColorAttachmentBuilder(Builder *p_Builder);

        ColorAttachmentBuilder &EnableBlending();
        ColorAttachmentBuilder &DisableBlending();

        ColorAttachmentBuilder &SetColorWriteMask(VkColorComponentFlags p_Mask);

        ColorAttachmentBuilder &SetColorBlendFactors(VkBlendFactor p_SrcColor, VkBlendFactor p_DstColor);
        ColorAttachmentBuilder &SetColorBlendOperation(VkBlendOp p_ColorOp);

        ColorAttachmentBuilder &SetAlphaBlendFactors(VkBlendFactor p_SrcAlpha, VkBlendFactor p_DstAlpha);
        ColorAttachmentBuilder &SetAlphaBlendOperation(VkBlendOp p_AlphaOp);

        Builder &EndColorAttachment();

      private:
        Builder *m_Builder;
        VkPipelineColorBlendAttachmentState m_ColorBlendAttachmentInfo{};

        friend class Builder;
    };

  public:
    /**
     * @brief Builder for creating a Vulkan graphics pipeline.
     *
     * Contains all the necessary settings for pipeline creation, including shaders,
     * layout, render pass, and state settings. Provides utility methods for internal
     * setup and pipeline creation.
     */
    class Builder
    {
      public:
        using Flags = u8;
        enum FlagBits : Flags
        {
            Flag_None = 0,
            Flag_StencilFront = 1 << 0,
            Flag_StencilBack = 1 << 1,
        };

        Builder(const LogicalDevice::Proxy &p_Device, VkPipelineLayout p_Layout, VkRenderPass p_RenderPass,
                u32 p_Subpass = 0);
        Builder(const LogicalDevice::Proxy &p_Device, VkPipelineLayout p_Layout,
                const VkPipelineRenderingCreateInfoKHR &p_RenderingInfo);

        /**
         * @brief Builds the graphics pipeline based on the current settings.
         *
         * Creates the Vulkan graphics pipeline using the current configuration.
         * This method is used to finalize the pipeline and prepare it for use.
         *
         * Take into account that this method cannot be marked const because internal linkage must
         * happen to create the pipeline. Still, the builder will be left in a valid state after the call.
         *
         * @param p_Device The logical device proxy for Vulkan operations.
         * @return A `Result` containing the created `GraphicsPipeline` or an error if the creation fails.
         */
        Result<GraphicsPipeline> Build();

        /**
         * @brief Generates the `VkGraphicsPipelineCreateInfo` object.
         *
         * Constructs the Vulkan graphics pipeline creation info based on the specs.
         * This is used internally during pipeline creation.
         *
         * @return A `VkGraphicsPipelineCreateInfo` that represents the current pipeline configuration.
         */
        VkGraphicsPipelineCreateInfo CreatePipelineInfo();

        Builder &SetBasePipeline(VkPipeline p_BasePipeline);
        Builder &SetBasePipelineIndex(i32 p_BasePipelineIndex);
        Builder &SetCache(VkPipelineCache p_Cache);

        // Input Assembly
        Builder &SetTopology(VkPrimitiveTopology p_Topology);
        Builder &EnablePrimitiveRestart();
        Builder &DisablePrimitiveRestart();

        // Viewport and Scissor
        Builder &AddViewport(VkViewport p_Viewport, VkRect2D p_Scissor);
        Builder &AddViewports(TKit::Span<std::pair<VkViewport, VkRect2D>> p_Viewports);
        Builder &SetViewports(TKit::Span<std::pair<VkViewport, VkRect2D>> p_Viewports);
        Builder &SetViewportCount(u32 p_ViewportCount);

        // Rasterization
        Builder &EnableRasterizerDiscard();
        Builder &EnableDepthClamp();
        Builder &EnableDepthBias();
        Builder &DisableRasterizerDiscard();
        Builder &DisableDepthClamp();
        Builder &DisableDepthBias();
        Builder &SetPolygonMode(VkPolygonMode p_Mode);
        Builder &SetLineWidth(f32 p_Width);
        Builder &SetCullMode(VkCullModeFlags p_Mode);
        Builder &SetFrontFace(VkFrontFace p_FrontFace);
        Builder &SetDepthBias(f32 p_ConstantFactor, f32 p_Clamp, f32 p_SlopeFactor);

        // Multisampling
        Builder &EnableSampleShading();
        Builder &EnableAlphaToCoverage();
        Builder &EnableAlphaToOne();
        Builder &DisableSampleShading();
        Builder &DisableAlphaToCoverage();
        Builder &DisableAlphaToOne();
        Builder &SetSampleCount(VkSampleCountFlagBits p_SampleCount);
        Builder &SetMinSampleShading(f32 p_MinSampleShading);
        Builder &SetSampleMask(const VkSampleMask *p_SampleMask);

        // Color Blending
        Builder &EnableLogicOperation();
        Builder &DisableLogicOperation();
        Builder &SetLogicOperation(VkLogicOp p_Operation);
        Builder &SetBlendConstants(const f32 *p_Constants);
        Builder &SetBlendConstants(f32 p_C1, f32 p_C2, f32 p_C3, f32 p_C4);
        Builder &SetBlendConstant(u32 p_Index, f32 p_Value);
        Builder &AddDefaultColorAttachment();
        ColorAttachmentBuilder &BeginColorAttachment();

        // Depth and Stencil
        Builder &EnableDepthTest();
        Builder &EnableDepthWrite();
        Builder &EnableDepthBoundsTest();
        Builder &EnableStencilTest();
        Builder &DisableDepthTest();
        Builder &DisableDepthWrite();
        Builder &DisableDepthBoundsTest();
        Builder &DisableStencilTest();
        Builder &SetDepthCompareOperation(VkCompareOp p_Op);
        Builder &SetDepthBounds(f32 p_Min, f32 p_Max);
        Builder &SetStencilFailOperation(VkStencilOp p_FailOp, Flags p_Flags);
        Builder &SetStencilPassOperation(VkStencilOp p_PassOp, Flags p_Flags);
        Builder &SetStencilDepthFailOperation(VkStencilOp p_DepthFailOp, Flags p_Flags);
        Builder &SetStencilCompareOperation(VkCompareOp p_CompareOp, Flags p_Flags);
        Builder &SetStencilCompareMask(u32 p_Mask, Flags p_Flags);
        Builder &SetStencilWriteMask(u32 p_Mask, Flags p_Flags);
        Builder &SetStencilReference(u32 p_Reference, Flags p_Flags);

        // Vertex Input
        Builder &AddBindingDescription(VkVertexInputRate p_InputRate, u32 p_Stride);
        template <typename T> Builder &AddBindingDescription(const VkVertexInputRate p_InputRate)
        {
            AddBindingDescription(p_InputRate, sizeof(T));
            return *this;
        }
        Builder &AddAttributeDescription(u32 p_Binding, VkFormat p_Format, u32 p_Offset);

        // Shader Stages
        Builder &AddShaderStage(VkShaderModule p_Module, VkShaderStageFlagBits p_Stage,
                                VkPipelineShaderStageCreateFlags p_Flags = 0,
                                const VkSpecializationInfo *p_Info = nullptr, const char *p_EntryPoint = "main");

        // Dynamic State
        Builder &AddDynamicState(VkDynamicState p_State);
        Builder &AddDynamicStates(TKit::Span<const VkDynamicState> p_States);
        Builder &SetDynamicStates(TKit::Span<const VkDynamicState> p_States);

      private:
        void initialize();
        LogicalDevice::Proxy m_Device;
        VkPipelineLayout m_Layout;
        VkRenderPass m_RenderPass;

        VkPipelineRenderingCreateInfoKHR m_RenderingInfo{};
        VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyInfo{};
        VkPipelineViewportStateCreateInfo m_ViewportInfo{};
        VkPipelineRasterizationStateCreateInfo m_RasterizationInfo{};
        VkPipelineMultisampleStateCreateInfo m_MultisampleInfo{};
        VkPipelineColorBlendStateCreateInfo m_ColorBlendInfo{};
        VkPipelineDepthStencilStateCreateInfo m_DepthStencilInfo{};
        VkPipelineVertexInputStateCreateInfo m_VertexInputInfo{};
        VkPipelineDynamicStateCreateInfo m_DynamicStateInfo{};

        VkPipeline m_BasePipeline = VK_NULL_HANDLE;
        VkPipelineCache m_Cache = VK_NULL_HANDLE;
        i32 m_BasePipelineIndex = -1;

        u32 m_Subpass;

        TKit::StaticArray128<VkDynamicState> m_DynamicStates;
        TKit::StaticArray16<VkVertexInputBindingDescription> m_BindingDescriptions;
        TKit::StaticArray16<VkVertexInputAttributeDescription> m_AttributeDescriptions;
        TKit::StaticArray8<ColorAttachmentBuilder> m_ColorAttachmentBuilders;

        // This array may sound redundant, but it is needed because this builder allows to generate a
        // PipelineCreateInfo, and so linked arrays to the create info must remain alive.
        TKit::StaticArray8<VkPipelineColorBlendAttachmentState> m_ColorAttachments;
        TKit::StaticArray4<VkPipelineShaderStageCreateInfo> m_ShaderStages;
        TKit::StaticArray4<std::pair<VkViewport, VkRect2D>> m_Viewports;

        friend class ColorAttachmentBuilder;
    };

    /**
     * @brief Creates multiple graphics pipelines in a batch.
     *
     * Initializes multiple Vulkan graphics pipelines using the provided specifications
     * and logical device.
     *
     * @param p_Device The logical device proxy for Vulkan operations.
     * @param p_Specs A span of pipeline builders containing the specifications for each pipeline.
     * @param p_Pipelines A span to store the created pipelines.
     * @return A `Result` indicating success or failure for the batch operation.
     */
    static Result<> Create(const LogicalDevice::Proxy &p_Device, TKit::Span<Builder> p_Builders,
                           TKit::Span<GraphicsPipeline> p_Pipelines, VkPipelineCache p_Cache = VK_NULL_HANDLE);

    GraphicsPipeline() = default;
    GraphicsPipeline(const LogicalDevice::Proxy &p_Device, const VkPipeline p_Pipeline)
        : m_Device(p_Device), m_Pipeline(p_Pipeline)
    {
    }

    void Destroy();
    void SubmitForDeletion(DeletionQueue &p_Queue) const;

    /**
     * @brief Binds the graphics pipeline to a command buffer.
     *
     * Prepares the pipeline for rendering by binding it to the specified command buffer.
     *
     * @param p_CommandBuffer The Vulkan command buffer to bind the pipeline to.
     */
    void Bind(VkCommandBuffer p_CommandBuffer) const;

    const LogicalDevice::Proxy &GetDevice() const
    {
        return m_Device;
    }
    VkPipeline GetHandle() const
    {
        return m_Pipeline;
    }
    operator VkPipeline() const
    {
        return m_Pipeline;
    }
    operator bool() const
    {
        return m_Pipeline != VK_NULL_HANDLE;
    }

  private:
    LogicalDevice::Proxy m_Device{};
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};
} // namespace VKit
