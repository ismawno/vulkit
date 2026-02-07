#pragma once

#ifndef VKIT_ENABLE_GRAPHICS_PIPELINE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_GRAPHICS_PIPELINE"
#endif

#include "vkit/device/proxy_device.hpp"
#include "tkit/container/span.hpp"
#include <vulkan/vulkan.h>

namespace VKit
{
using StencilOperationFlags = u8;
enum StencilOperationFlagBits : StencilOperationFlags
{
    StencilOperationFlag_Front = 1 << 0,
    StencilOperationFlag_Back = 1 << 1,
};
// no pNext hooks for now. must retrieve create info and add them yourself
class GraphicsPipeline
{
  public:
    class Builder;

  private:
    class ColorAttachmentBuilder
    {
      public:
        ColorAttachmentBuilder(Builder *builder);

        ColorAttachmentBuilder &EnableBlending();
        ColorAttachmentBuilder &DisableBlending();

        ColorAttachmentBuilder &SetColorWriteMask(VkColorComponentFlags mask);

        ColorAttachmentBuilder &SetColorBlendFactors(VkBlendFactor srcColor, VkBlendFactor dstColor);
        ColorAttachmentBuilder &SetColorBlendOperation(VkBlendOp colorOp);

        ColorAttachmentBuilder &SetAlphaBlendFactors(VkBlendFactor srcAlpha, VkBlendFactor dstAlpha);
        ColorAttachmentBuilder &SetAlphaBlendOperation(VkBlendOp alphaOp);

        Builder &EndColorAttachment();

      private:
        Builder *m_Builder;
        VkPipelineColorBlendAttachmentState m_ColorBlendAttachmentInfo{};

        friend class Builder;
    };

    struct ViewportInfo
    {
        VkViewport Viewport;
        VkRect2D Scissor;
    };

  public:
    class Builder
    {
      public:
        Builder(const ProxyDevice &device, VkPipelineLayout layout, VkRenderPass renderPass, u32 subpass = 0);
        Builder(const ProxyDevice &device, VkPipelineLayout layout,
                const VkPipelineRenderingCreateInfoKHR &renderingInfo);

        /**
         * @brief Builds the graphics pipeline based on the current settings.
         *
         * Creates the Vulkan graphics pipeline using the current configuration.
         *
         * IMPORTANT: `Bake()` method must be called before calling `Build()` if changes were made to the builder.
         *
         * @return A `Result` containing the created `GraphicsPipeline` or an error if the creation fails.
         */
        VKIT_NO_DISCARD Result<GraphicsPipeline> Build() const;

        /**
         * @brief Generates the `VkGraphicsPipelineCreateInfo` object.
         *
         * Constructs the Vulkan graphics pipeline creation info based on the specs.
         * This is used internally during pipeline creation.
         *
         * IMPORTANT: `Bake()` method must be called before calling this method to ensure a consistent state and avoid
         * dangling references. Builder must be kept alive while the info struct is active.
         *
         * @return A `VkGraphicsPipelineCreateInfo` that represents the current pipeline configuration.
         */
        VkGraphicsPipelineCreateInfo CreatePipelineInfo() const;

        Builder &Bake();
        Builder &SetBasePipeline(VkPipeline basePipeline);
        Builder &SetBasePipelineIndex(i32 basePipelineIndex);
        Builder &SetCache(VkPipelineCache cache);

        // Input Assembly
        Builder &SetTopology(VkPrimitiveTopology topology);
        Builder &EnablePrimitiveRestart();
        Builder &DisablePrimitiveRestart();

        // Viewport and Scissor
        Builder &AddViewport(VkViewport viewport, VkRect2D scissor);
        Builder &SetViewportCount(u32 viewportCount);

        // Rasterization
        Builder &EnableRasterizerDiscard();
        Builder &EnableDepthClamp();
        Builder &EnableDepthBias();
        Builder &DisableRasterizerDiscard();
        Builder &DisableDepthClamp();
        Builder &DisableDepthBias();
        Builder &SetPolygonMode(VkPolygonMode mode);
        Builder &SetLineWidth(f32 width);
        Builder &SetCullMode(VkCullModeFlags mode);
        Builder &SetFrontFace(VkFrontFace frontFace);
        Builder &SetDepthBias(f32 constantFactor, f32 clamp, f32 slopeFactor);

        // Multisampling
        Builder &EnableSampleShading();
        Builder &EnableAlphaToCoverage();
        Builder &EnableAlphaToOne();
        Builder &DisableSampleShading();
        Builder &DisableAlphaToCoverage();
        Builder &DisableAlphaToOne();
        Builder &SetSampleCount(VkSampleCountFlagBits sampleCount);
        Builder &SetMinSampleShading(f32 minSampleShading);
        Builder &SetSampleMask(const VkSampleMask *sampleMask);

        // Color Blending
        Builder &EnableLogicOperation();
        Builder &DisableLogicOperation();
        Builder &SetLogicOperation(VkLogicOp operation);
        Builder &SetBlendConstants(const f32 *constants);
        Builder &SetBlendConstants(f32 c1, f32 c2, f32 c3, f32 c4);
        Builder &SetBlendConstant(u32 index, f32 value);
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
        Builder &SetDepthCompareOperation(VkCompareOp op);
        Builder &SetDepthBounds(f32 min, f32 max);
        Builder &SetStencilFailOperation(VkStencilOp failOp, StencilOperationFlags flags);
        Builder &SetStencilPassOperation(VkStencilOp passOp, StencilOperationFlags flags);
        Builder &SetStencilDepthFailOperation(VkStencilOp depthFailOp, StencilOperationFlags flags);
        Builder &SetStencilCompareOperation(VkCompareOp compareOp, StencilOperationFlags flags);
        Builder &SetStencilCompareMask(u32 mask, StencilOperationFlags flags);
        Builder &SetStencilWriteMask(u32 mask, StencilOperationFlags flags);
        Builder &SetStencilReference(u32 reference, StencilOperationFlags flags);

        // Vertex Input
        Builder &AddBindingDescription(u32 stride, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);
        template <typename T>
        Builder &AddBindingDescription(const VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX)
        {
            AddBindingDescription(sizeof(T), inputRate);
            return *this;
        }
        Builder &AddAttributeDescription(u32 binding, VkFormat format, u32 offset);

        // Shader Stages
        Builder &AddShaderStage(VkShaderModule module, VkShaderStageFlagBits stage,
                                VkPipelineShaderStageCreateFlags flags = 0, const VkSpecializationInfo *info = nullptr,
                                const char *entryPoint = "main");

        // Dynamic State
        Builder &AddDynamicState(VkDynamicState state);

      private:
        void initialize();
        ProxyDevice m_Device;
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

        TKit::TierArray<VkDynamicState> m_DynamicStates{};
        TKit::TierArray<VkVertexInputBindingDescription> m_BindingDescriptions{};
        TKit::TierArray<VkVertexInputAttributeDescription> m_AttributeDescriptions{};
        TKit::TierArray<ColorAttachmentBuilder> m_ColorAttachmentBuilders{};

        // This array may sound redundant, but it is needed because this builder allows to generate a
        // PipelineCreateInfo, and so linked arrays to the create info must remain alive.
        TKit::TierArray<VkPipelineColorBlendAttachmentState> m_ColorAttachments{};
        TKit::TierArray<VkPipelineShaderStageCreateInfo> m_ShaderStages{};
        TKit::TierArray<ViewportInfo> m_Viewports{};

        friend class ColorAttachmentBuilder;
    };

    VKIT_NO_DISCARD static Result<> Create(const ProxyDevice &device, TKit::Span<const Builder> builders,
                                           TKit::Span<GraphicsPipeline> pipelines,
                                           VkPipelineCache cache = VK_NULL_HANDLE);

    GraphicsPipeline() = default;
    GraphicsPipeline(const ProxyDevice &device, const VkPipeline pipeline) : m_Device(device), m_Pipeline(pipeline)
    {
    }

    void Destroy();

    void Bind(VkCommandBuffer commandBuffer) const;

    const ProxyDevice &GetDevice() const
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
    ProxyDevice m_Device{};
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};
} // namespace VKit
