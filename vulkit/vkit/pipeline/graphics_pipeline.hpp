#pragma once

#include "vkit/core/alias.hpp"
#include "vkit/backend/logical_device.hpp"
#include "vkit/pipeline/shader.hpp"
#include "tkit/utils/non_copyable.hpp"

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
        explicit ColorAttachmentBuilder(Builder *p_Builder) noexcept;

        ColorAttachmentBuilder &EnableBlending() noexcept;
        ColorAttachmentBuilder &DisableBlending() noexcept;

        ColorAttachmentBuilder &SetColorWriteMask(VkColorComponentFlags p_Mask) noexcept;

        ColorAttachmentBuilder &SetColorBlendFactors(VkBlendFactor p_SrcColor, VkBlendFactor p_DstColor) noexcept;
        ColorAttachmentBuilder &SetColorBlendOperation(VkBlendOp p_ColorOp) noexcept;

        ColorAttachmentBuilder &SetAlphaBlendFactors(VkBlendFactor p_SrcAlpha, VkBlendFactor p_DstAlpha) noexcept;
        ColorAttachmentBuilder &SetAlphaBlendOperation(VkBlendOp p_AlphaOp) noexcept;

        Builder &EndColorAttachment() noexcept;

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
        enum FlagBit : Flags
        {
            Flag_StencilFront = 1 << 0,
            Flag_StencilBack = 1 << 1,
        };

        Builder(const LogicalDevice::Proxy &p_Device, VkPipelineLayout p_Layout, VkRenderPass p_RenderPass,
                u32 p_Subpass = 0) noexcept;

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
        Result<GraphicsPipeline> Build() noexcept;

        /**
         * @brief Generates the `VkGraphicsPipelineCreateInfo` object.
         *
         * Constructs the Vulkan graphics pipeline creation info based on the specs.
         * This is used internally during pipeline creation.
         *
         * @return A `VkGraphicsPipelineCreateInfo` that represents the current pipeline configuration.
         */
        VkGraphicsPipelineCreateInfo CreatePipelineInfo() noexcept;

        Builder &SetBasePipeline(VkPipeline p_BasePipeline) noexcept;
        Builder &SetBasePipelineIndex(i32 p_BasePipelineIndex) noexcept;
        Builder &SetCache(VkPipelineCache p_Cache) noexcept;

        // Input Assembly
        Builder &SetTopology(VkPrimitiveTopology p_Topology) noexcept;
        Builder &EnablePrimitiveRestart() noexcept;
        Builder &DisablePrimitiveRestart() noexcept;

        // Viewport and Scissor
        Builder &AddViewport(VkViewport p_Viewport, VkRect2D p_Scissor) noexcept;
        Builder &AddViewports(TKit::Span<std::pair<VkViewport, VkRect2D>> p_Viewports) noexcept;
        Builder &SetViewports(TKit::Span<std::pair<VkViewport, VkRect2D>> p_Viewports) noexcept;
        Builder &SetViewportCount(u32 p_ViewportCount) noexcept;

        // Rasterization
        Builder &EnableRasterizerDiscard() noexcept;
        Builder &EnableDepthClamp() noexcept;
        Builder &EnableDepthBias() noexcept;
        Builder &DisableRasterizerDiscard() noexcept;
        Builder &DisableDepthClamp() noexcept;
        Builder &DisableDepthBias() noexcept;
        Builder &SetPolygonMode(VkPolygonMode p_Mode) noexcept;
        Builder &SetLineWidth(f32 p_Width) noexcept;
        Builder &SetCullMode(VkCullModeFlags p_Mode) noexcept;
        Builder &SetFrontFace(VkFrontFace p_FrontFace) noexcept;
        Builder &SetDepthBias(f32 p_ConstantFactor, f32 p_Clamp, f32 p_SlopeFactor) noexcept;

        // Multisampling
        Builder &EnableSampleShading() noexcept;
        Builder &EnableAlphaToCoverage() noexcept;
        Builder &EnableAlphaToOne() noexcept;
        Builder &DisableSampleShading() noexcept;
        Builder &DisableAlphaToCoverage() noexcept;
        Builder &DisableAlphaToOne() noexcept;
        Builder &SetSampleCount(VkSampleCountFlagBits p_SampleCount) noexcept;
        Builder &SetMinSampleShading(f32 p_MinSampleShading) noexcept;
        Builder &SetSampleMask(const VkSampleMask *p_SampleMask) noexcept;

        // Color Blending
        Builder &EnableLogicOperation() noexcept;
        Builder &DisableLogicOperation() noexcept;
        Builder &SetLogicOperation(VkLogicOp p_Operation) noexcept;
        Builder &SetBlendConstants(const f32 *p_Constants) noexcept;
        Builder &SetBlendConstants(f32 p_C1, f32 p_C2, f32 p_C3, f32 p_C4) noexcept;
        Builder &SetBlendConstant(u32 p_Index, f32 p_Value) noexcept;
        Builder &AddDefaultColorAttachment() noexcept;
        ColorAttachmentBuilder &BeginColorAttachment() noexcept;

        // Depth and Stencil
        Builder &EnableDepthTest() noexcept;
        Builder &EnableDepthWrite() noexcept;
        Builder &EnableDepthBoundsTest() noexcept;
        Builder &EnableStencilTest() noexcept;
        Builder &DisableDepthTest() noexcept;
        Builder &DisableDepthWrite() noexcept;
        Builder &DisableDepthBoundsTest() noexcept;
        Builder &DisableStencilTest() noexcept;
        Builder &SetDepthCompareOperation(VkCompareOp p_Op) noexcept;
        Builder &SetDepthBounds(f32 p_Min, f32 p_Max) noexcept;
        Builder &SetStencilFailOperation(VkStencilOp p_FailOp, Flags p_Flags) noexcept;
        Builder &SetStencilPassOperation(VkStencilOp p_PassOp, Flags p_Flags) noexcept;
        Builder &SetStencilDepthFailOperation(VkStencilOp p_DepthFailOp, Flags p_Flags) noexcept;
        Builder &SetStencilCompareOperation(VkCompareOp p_CompareOp, Flags p_Flags) noexcept;
        Builder &SetStencilCompareMask(u32 p_Mask, Flags p_Flags) noexcept;
        Builder &SetStencilWriteMask(u32 p_Mask, Flags p_Flags) noexcept;
        Builder &SetStencilReference(u32 p_Reference, Flags p_Flags) noexcept;

        // Vertex Input
        Builder &AddBindingDescription(VkVertexInputRate p_InputRate, u32 p_Stride) noexcept;
        template <typename T> Builder &AddBindingDescription(const VkVertexInputRate p_InputRate) noexcept
        {
            AddBindingDescription(p_InputRate, sizeof(T));
            return *this;
        }
        Builder &AddAttributeDescription(u32 p_Binding, VkFormat p_Format, u32 p_Offset) noexcept;

        // Shader Stages
        Builder &AddShaderStage(VkShaderModule p_Module, VkShaderStageFlagBits p_Stage,
                                VkPipelineShaderStageCreateFlags p_Flags = 0,
                                const VkSpecializationInfo *p_Info = nullptr,
                                const char *p_EntryPoint = "main") noexcept;

        // Dynamic State
        Builder &AddDynamicState(VkDynamicState p_State) noexcept;
        Builder &AddDynamicStates(TKit::Span<const VkDynamicState> p_States) noexcept;
        Builder &SetDynamicStates(TKit::Span<const VkDynamicState> p_States) noexcept;

      private:
        LogicalDevice::Proxy m_Device;

        VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyInfo{};
        VkPipelineViewportStateCreateInfo m_ViewportInfo{};
        VkPipelineRasterizationStateCreateInfo m_RasterizationInfo{};
        VkPipelineMultisampleStateCreateInfo m_MultisampleInfo{};
        VkPipelineColorBlendStateCreateInfo m_ColorBlendInfo{};
        VkPipelineDepthStencilStateCreateInfo m_DepthStencilInfo{};
        VkPipelineVertexInputStateCreateInfo m_VertexInputInfo{};
        VkPipelineDynamicStateCreateInfo m_DynamicStateInfo{};

        VkPipelineLayout m_Layout;
        VkRenderPass m_RenderPass;

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
     * @return A VulkanResult indicating success or failure for the batch operation.
     */
    static VulkanResult Create(const LogicalDevice::Proxy &p_Device, TKit::Span<Builder> p_Builders,
                               TKit::Span<GraphicsPipeline> p_Pipelines,
                               VkPipelineCache p_Cache = VK_NULL_HANDLE) noexcept;

    GraphicsPipeline() noexcept = default;
    GraphicsPipeline(const LogicalDevice::Proxy &p_Device, VkPipeline p_Pipeline) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    /**
     * @brief Binds the graphics pipeline to a command buffer.
     *
     * Prepares the pipeline for rendering by binding it to the specified command buffer.
     *
     * @param p_CommandBuffer The Vulkan command buffer to bind the pipeline to.
     */
    void Bind(VkCommandBuffer p_CommandBuffer) const noexcept;

    /**
     * @brief Retrieves the pipeline layout.
     *
     * @return The Vulkan pipeline layout.
     */

    VkPipeline GetPipeline() const noexcept;
    explicit(false) operator VkPipeline() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};
} // namespace VKit
