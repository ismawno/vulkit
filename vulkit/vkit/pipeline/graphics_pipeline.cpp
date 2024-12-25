#include "vkit/core/pch.hpp"
#include "vkit/pipeline/graphics_pipeline.hpp"
#include "tkit/core/logging.hpp"
#include "tkit/memory/stack_allocator.hpp"

#include <filesystem>

namespace VKit
{
GraphicsPipeline::Builder::Builder(const LogicalDevice::Proxy &p_Device, const VkPipelineLayout p_Layout,
                                   const VkRenderPass p_RenderPass, const u32 p_Subpass) noexcept
    : m_Device(p_Device), m_Layout(p_Layout), m_RenderPass(p_RenderPass), m_Subpass(p_Subpass)
{
    m_InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    m_ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_ViewportInfo.viewportCount = 0;
    m_ViewportInfo.pViewports = nullptr;
    m_ViewportInfo.scissorCount = 0;
    m_ViewportInfo.pScissors = nullptr;

    m_RasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_RasterizationInfo.depthClampEnable = VK_FALSE;
    m_RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    m_RasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    m_RasterizationInfo.lineWidth = 1.0f;
    m_RasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    m_RasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    m_RasterizationInfo.depthBiasEnable = VK_FALSE;
    m_RasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
    m_RasterizationInfo.depthBiasClamp = 0.0f;          // Optional
    m_RasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

    m_MultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_MultisampleInfo.sampleShadingEnable = VK_FALSE;
    m_MultisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_MultisampleInfo.minSampleShading = 1.0f;          // Optional
    m_MultisampleInfo.pSampleMask = nullptr;            // Optional
    m_MultisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional
    m_MultisampleInfo.alphaToOneEnable = VK_FALSE;      // Optional

    m_ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_ColorBlendInfo.logicOpEnable = VK_FALSE;
    m_ColorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    m_ColorBlendInfo.blendConstants[0] = 0.0f;   // Optional
    m_ColorBlendInfo.blendConstants[1] = 0.0f;   // Optional
    m_ColorBlendInfo.blendConstants[2] = 0.0f;   // Optional
    m_ColorBlendInfo.blendConstants[3] = 0.0f;   // Optional
    m_ColorBlendInfo.pAttachments = nullptr;
    m_ColorBlendInfo.attachmentCount = 0;

    m_DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_DepthStencilInfo.depthTestEnable = VK_FALSE;
    m_DepthStencilInfo.depthWriteEnable = VK_FALSE;
    m_DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    m_DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencilInfo.minDepthBounds = 0.0f; // Optional
    m_DepthStencilInfo.maxDepthBounds = 1.0f; // Optional
    m_DepthStencilInfo.stencilTestEnable = VK_FALSE;
    m_DepthStencilInfo.front = {}; // Optional
    m_DepthStencilInfo.back = {};  // Optional

    m_DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_DynamicStateInfo.pDynamicStates = nullptr;
    m_DynamicStateInfo.dynamicStateCount = 0;

    m_VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
}

Result<GraphicsPipeline> GraphicsPipeline::Builder::Build() noexcept
{
    const VkGraphicsPipelineCreateInfo pipelineInfo = CreatePipelineInfo();
    VkPipeline pipeline;

    const VkResult result =
        vkCreateGraphicsPipelines(m_Device, m_Cache, 1, &pipelineInfo, m_Device.AllocationCallbacks, &pipeline);
    if (result != VK_SUCCESS)
        return Result<GraphicsPipeline>::Error(result, "Failed to create graphics pipeline");

    return Result<GraphicsPipeline>::Ok(m_Device, pipeline);
}

VulkanResult GraphicsPipeline::Create(const LogicalDevice::Proxy &p_Device, const std::span<Builder> p_Builders,
                                      const std::span<GraphicsPipeline> p_Pipelines,
                                      const VkPipelineCache p_Cache) noexcept
{
    if (p_Builders.size() != p_Pipelines.size())
        return VulkanResult::Error(VK_ERROR_INITIALIZATION_FAILED, "Specs and pipelines must have the same size");
    if (p_Builders.size() == 0)
        return VulkanResult::Error(VK_ERROR_INITIALIZATION_FAILED, "Specs and pipelines must not be empty");

    TKit::StaticArray32<VkGraphicsPipelineCreateInfo> pipelineInfos;
    for (Builder &builder : p_Builders)
        pipelineInfos.push_back(builder.CreatePipelineInfo());

    TKit::StaticArray32<VkPipeline> pipelines{p_Builders.size()};
    const VkResult result =
        vkCreateGraphicsPipelines(p_Device, p_Cache, static_cast<u32>(p_Builders.size()), pipelineInfos.data(),
                                  p_Device.AllocationCallbacks, pipelines.data());

    if (result != VK_SUCCESS)
        return VulkanResult::Error(result, "Failed to create graphics pipelines");

    for (usize i = 0; i < p_Builders.size(); ++i)
        p_Pipelines[i] = GraphicsPipeline(p_Device, pipelines[i]);

    return VulkanResult::Success();
}

GraphicsPipeline::GraphicsPipeline(const LogicalDevice::Proxy &p_Device, const VkPipeline p_Pipeline) noexcept
    : m_Device(p_Device), m_Pipeline(p_Pipeline)
{
}

void GraphicsPipeline::Destroy() noexcept
{
    TKIT_ASSERT(m_Pipeline, "The graphics pipeline is a NULL handle");
    vkDestroyPipeline(m_Device, m_Pipeline, m_Device.AllocationCallbacks);
    m_Pipeline = VK_NULL_HANDLE;
}
void GraphicsPipeline::SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
{
    const VkPipeline pipeline = m_Pipeline;
    const LogicalDevice::Proxy device = m_Device;
    p_Queue.Push([pipeline, device]() { vkDestroyPipeline(device, pipeline, device.AllocationCallbacks); });
}

void GraphicsPipeline::Bind(VkCommandBuffer p_CommandBuffer) const noexcept
{
    vkCmdBindPipeline(p_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
}

VkPipeline GraphicsPipeline::GetPipeline() const noexcept
{
    return m_Pipeline;
}
GraphicsPipeline::operator VkPipeline() const noexcept
{
    return m_Pipeline;
}
GraphicsPipeline::operator bool() const noexcept
{
    return m_Pipeline != VK_NULL_HANDLE;
}

VkGraphicsPipelineCreateInfo GraphicsPipeline::Builder::CreatePipelineInfo() noexcept
{
    m_ColorBlendInfo.attachmentCount = static_cast<u32>(m_ColorAttachments.size());
    m_ColorBlendInfo.pAttachments = m_ColorAttachments.empty() ? nullptr : m_ColorAttachments.data();

    m_DynamicStateInfo.dynamicStateCount = static_cast<u32>(m_DynamicStates.size());
    m_DynamicStateInfo.pDynamicStates = m_DynamicStates.empty() ? nullptr : m_DynamicStates.data();

    m_VertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(m_AttributeDescriptions.size());
    m_VertexInputInfo.vertexBindingDescriptionCount = static_cast<u32>(m_BindingDescriptions.size());
    m_VertexInputInfo.pVertexAttributeDescriptions =
        m_AttributeDescriptions.empty() ? nullptr : m_AttributeDescriptions.data();
    m_VertexInputInfo.pVertexBindingDescriptions =
        m_BindingDescriptions.empty() ? nullptr : m_BindingDescriptions.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<u32>(m_ShaderStages.size());
    pipelineInfo.pStages = m_ShaderStages.empty() ? nullptr : m_ShaderStages.data();
    pipelineInfo.pVertexInputState = &m_VertexInputInfo;
    pipelineInfo.pInputAssemblyState = &m_InputAssemblyInfo;
    pipelineInfo.pViewportState = &m_ViewportInfo;
    pipelineInfo.pRasterizationState = &m_RasterizationInfo;
    pipelineInfo.pMultisampleState = &m_MultisampleInfo;
    pipelineInfo.pColorBlendState = &m_ColorBlendInfo;
    pipelineInfo.pDepthStencilState = &m_DepthStencilInfo;
    pipelineInfo.pDynamicState = &m_DynamicStateInfo;

    pipelineInfo.layout = m_Layout;
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = m_Subpass;

    pipelineInfo.basePipelineHandle = m_BasePipeline;
    pipelineInfo.basePipelineIndex = m_BasePipelineIndex;

    return pipelineInfo;
}

GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBasePipeline(const VkPipeline p_BasePipeline) noexcept
{
    m_BasePipeline = p_BasePipeline;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBasePipelineIndex(const i32 p_BasePipelineIndex) noexcept
{
    m_BasePipelineIndex = p_BasePipelineIndex;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetCache(const VkPipelineCache p_Cache) noexcept
{
    m_Cache = p_Cache;
    return *this;
}

// Input Assembly
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetTopology(const VkPrimitiveTopology p_Topology) noexcept
{
    m_InputAssemblyInfo.topology = p_Topology;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnablePrimitiveRestart() noexcept
{
    m_InputAssemblyInfo.primitiveRestartEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisablePrimitiveRestart() noexcept
{
    m_InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
    return *this;
}

// Viewport and Scissor
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddViewport(const VkViewport p_Viewport,
                                                                  const VkRect2D p_Scissor) noexcept
{
    m_Viewports.push_back(std::make_pair(p_Viewport, p_Scissor));
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddViewports(
    const std::span<std::pair<VkViewport, VkRect2D>> p_Viewports) noexcept
{
    for (const auto &viewport : p_Viewports)
        m_Viewports.push_back(viewport);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetViewports(
    const std::span<std::pair<VkViewport, VkRect2D>> p_Viewports) noexcept
{
    m_Viewports.clear();
    AddViewports(p_Viewports);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetViewportCount(const u32 p_ViewportCount) noexcept
{
    m_ViewportInfo.viewportCount = p_ViewportCount;
    m_ViewportInfo.scissorCount = p_ViewportCount;
    m_ViewportInfo.pViewports = nullptr;
    m_ViewportInfo.pScissors = nullptr;
    return *this;
}

// Rasterization
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableRasterizerDiscard() noexcept
{
    m_RasterizationInfo.rasterizerDiscardEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableDepthClamp() noexcept
{
    m_RasterizationInfo.depthClampEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableRasterizerDiscard() noexcept
{
    m_RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableDepthClamp() noexcept
{
    m_RasterizationInfo.depthClampEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableDepthBias() noexcept
{
    m_RasterizationInfo.depthBiasEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetPolygonMode(const VkPolygonMode p_Mode) noexcept
{
    m_RasterizationInfo.polygonMode = p_Mode;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetLineWidth(const f32 p_Width) noexcept
{
    m_RasterizationInfo.lineWidth = p_Width;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetCullMode(const VkCullModeFlags p_Mode) noexcept
{
    m_RasterizationInfo.cullMode = p_Mode;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetFrontFace(const VkFrontFace p_FrontFace) noexcept
{
    m_RasterizationInfo.frontFace = p_FrontFace;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableDepthBias() noexcept
{
    m_RasterizationInfo.depthBiasEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetDepthBias(const f32 p_ConstantFactor, const f32 p_Clamp,
                                                                   const f32 p_SlopeFactor) noexcept
{
    m_RasterizationInfo.depthBiasConstantFactor = p_ConstantFactor;
    m_RasterizationInfo.depthBiasClamp = p_Clamp;
    m_RasterizationInfo.depthBiasSlopeFactor = p_SlopeFactor;
    return *this;
}

// Multisampling
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableSampleShading() noexcept
{
    m_MultisampleInfo.sampleShadingEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableSampleShading() noexcept
{
    m_MultisampleInfo.sampleShadingEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetSampleCount(const VkSampleCountFlagBits p_SampleCount) noexcept
{
    m_MultisampleInfo.rasterizationSamples = p_SampleCount;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetMinSampleShading(const f32 p_MinSampleShading) noexcept
{
    m_MultisampleInfo.minSampleShading = p_MinSampleShading;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetSampleMask(const VkSampleMask *p_SampleMask) noexcept
{
    m_MultisampleInfo.pSampleMask = p_SampleMask;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableAlphaToCoverage() noexcept
{
    m_MultisampleInfo.alphaToCoverageEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableAlphaToOne() noexcept
{
    m_MultisampleInfo.alphaToOneEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableAlphaToCoverage() noexcept
{
    m_MultisampleInfo.alphaToCoverageEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableAlphaToOne() noexcept
{
    m_MultisampleInfo.alphaToOneEnable = VK_FALSE;
    return *this;
}

// Color Blending
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableLogicOperation() noexcept
{
    m_ColorBlendInfo.logicOpEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableLogicOperation() noexcept
{
    m_ColorBlendInfo.logicOpEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetLogicOperation(const VkLogicOp p_Operation) noexcept
{
    m_ColorBlendInfo.logicOp = p_Operation;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBlendConstants(const f32 *p_Constants) noexcept
{
    m_ColorBlendInfo.blendConstants[0] = p_Constants[0];
    m_ColorBlendInfo.blendConstants[1] = p_Constants[1];
    m_ColorBlendInfo.blendConstants[2] = p_Constants[2];
    m_ColorBlendInfo.blendConstants[3] = p_Constants[3];
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBlendConstants(const f32 p_C1, const f32 p_C2, const f32 p_C3,
                                                                        const f32 p_C4) noexcept
{
    m_ColorBlendInfo.blendConstants[0] = p_C1;
    m_ColorBlendInfo.blendConstants[1] = p_C2;
    m_ColorBlendInfo.blendConstants[2] = p_C3;
    m_ColorBlendInfo.blendConstants[3] = p_C4;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBlendConstant(const u32 p_Index, const f32 p_Value) noexcept
{
    m_ColorBlendInfo.blendConstants[p_Index] = p_Value;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddDefaultColorAttachment() noexcept
{
    m_ColorAttachmentBuilders.emplace_back(this);
    m_ColorAttachments.push_back(m_ColorAttachmentBuilders.back().m_ColorBlendAttachmentInfo);
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::Builder::BeginColorAttachment() noexcept
{
    return m_ColorAttachmentBuilders.emplace_back(this);
}

// Depth and Stencil
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableDepthTest() noexcept
{
    m_DepthStencilInfo.depthTestEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableDepthWrite() noexcept
{
    m_DepthStencilInfo.depthWriteEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableDepthBoundsTest() noexcept
{
    m_DepthStencilInfo.depthBoundsTestEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableStencilTest() noexcept
{
    m_DepthStencilInfo.stencilTestEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableDepthTest() noexcept
{
    m_DepthStencilInfo.depthTestEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableDepthWrite() noexcept
{
    m_DepthStencilInfo.depthWriteEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableDepthBoundsTest() noexcept
{
    m_DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableStencilTest() noexcept
{
    m_DepthStencilInfo.stencilTestEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetDepthCompareOperation(const VkCompareOp p_Op) noexcept
{
    m_DepthStencilInfo.depthCompareOp = p_Op;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetDepthBounds(const f32 p_Min, const f32 p_Max) noexcept
{
    m_DepthStencilInfo.minDepthBounds = p_Min;
    m_DepthStencilInfo.maxDepthBounds = p_Max;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilFailOperation(const VkStencilOp p_FailOp,
                                                                              const Flags p_Flags) noexcept
{
    if (p_Flags & Flag_StencilFront)
        m_DepthStencilInfo.front.failOp = p_FailOp;
    if (p_Flags & Flag_StencilBack)
        m_DepthStencilInfo.back.failOp = p_FailOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilPassOperation(const VkStencilOp p_PassOp,
                                                                              const Flags p_Flags) noexcept
{
    if (p_Flags & Flag_StencilFront)
        m_DepthStencilInfo.front.passOp = p_PassOp;
    if (p_Flags & Flag_StencilBack)
        m_DepthStencilInfo.back.passOp = p_PassOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilDepthFailOperation(const VkStencilOp p_DepthFailOp,
                                                                                   const Flags p_Flags) noexcept
{
    if (p_Flags & Flag_StencilFront)
        m_DepthStencilInfo.front.depthFailOp = p_DepthFailOp;
    if (p_Flags & Flag_StencilBack)
        m_DepthStencilInfo.back.depthFailOp = p_DepthFailOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilCompareOperation(const VkCompareOp p_CompareOp,
                                                                                 const Flags p_Flags) noexcept
{
    if (p_Flags & Flag_StencilFront)
        m_DepthStencilInfo.front.compareOp = p_CompareOp;
    if (p_Flags & Flag_StencilBack)
        m_DepthStencilInfo.back.compareOp = p_CompareOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilCompareMask(const u32 p_Mask,
                                                                            const Flags p_Flags) noexcept
{
    if (p_Flags & Flag_StencilFront)
        m_DepthStencilInfo.front.compareMask = p_Mask;
    if (p_Flags & Flag_StencilBack)
        m_DepthStencilInfo.back.compareMask = p_Mask;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilWriteMask(const u32 p_Mask,
                                                                          const Flags p_Flags) noexcept
{
    if (p_Flags & Flag_StencilFront)
        m_DepthStencilInfo.front.writeMask = p_Mask;
    if (p_Flags & Flag_StencilBack)
        m_DepthStencilInfo.back.writeMask = p_Mask;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilReference(const u32 p_Reference,
                                                                          const Flags p_Flags) noexcept
{
    if (p_Flags & Flag_StencilFront)
        m_DepthStencilInfo.front.reference = p_Reference;
    if (p_Flags & Flag_StencilBack)
        m_DepthStencilInfo.back.reference = p_Reference;
    return *this;
}

// Vertex Input
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddBindingDescription(const VkVertexInputRate p_InputRate,
                                                                            const u32 p_Stride) noexcept
{
    VkVertexInputBindingDescription binding{};
    binding.binding = static_cast<u32>(m_BindingDescriptions.size());
    binding.stride = p_Stride;
    binding.inputRate = p_InputRate;
    m_BindingDescriptions.push_back(binding);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddAttributeDescription(const u32 p_Binding,
                                                                              const VkFormat p_Format,
                                                                              const u32 p_Offset) noexcept
{
    VkVertexInputAttributeDescription attribute{};
    attribute.binding = p_Binding;
    attribute.format = p_Format;
    attribute.location = static_cast<u32>(m_AttributeDescriptions.size());
    attribute.offset = p_Offset;
    m_AttributeDescriptions.push_back(attribute);
    return *this;
}

// Shader Stages
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddShaderStage(const VkShaderModule p_Module,
                                                                     const VkShaderStageFlagBits p_Stage,
                                                                     const VkPipelineShaderStageCreateFlags p_Flags,
                                                                     const VkSpecializationInfo *p_Info,
                                                                     const char *p_EntryPoint) noexcept
{
    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.module = p_Module;
    stage.stage = p_Stage;
    stage.flags = p_Flags;
    stage.pSpecializationInfo = p_Info;
    stage.pName = p_EntryPoint;
    m_ShaderStages.push_back(stage);
    return *this;
}

// Dynamic State
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddDynamicState(const VkDynamicState p_State) noexcept
{
    m_DynamicStates.push_back(p_State);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddDynamicStates(
    const std::span<const VkDynamicState> p_States) noexcept
{
    for (const VkDynamicState state : p_States)
        m_DynamicStates.push_back(state);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetDynamicStates(
    const std::span<const VkDynamicState> p_States) noexcept
{
    m_DynamicStates.clear();
    AddDynamicStates(p_States);
    return *this;
}

GraphicsPipeline::ColorAttachmentBuilder::ColorAttachmentBuilder(Builder *p_Builder) noexcept : m_Builder(p_Builder)
{
    m_ColorBlendAttachmentInfo.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_ColorBlendAttachmentInfo.blendEnable = VK_FALSE;
    m_ColorBlendAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;           // Optional
    m_ColorBlendAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
    m_ColorBlendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;                            // Optional
    m_ColorBlendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;                 // Optional
    m_ColorBlendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
    m_ColorBlendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;                            // Optional
}

GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::EnableBlending() noexcept
{
    m_ColorBlendAttachmentInfo.blendEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::DisableBlending() noexcept
{
    m_ColorBlendAttachmentInfo.blendEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetColorWriteMask(
    const VkColorComponentFlags p_WriteMask) noexcept
{
    m_ColorBlendAttachmentInfo.colorWriteMask = p_WriteMask;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetColorBlendFactors(
    const VkBlendFactor p_SrcColor, const VkBlendFactor p_DstColor) noexcept
{
    m_ColorBlendAttachmentInfo.srcColorBlendFactor = p_SrcColor;
    m_ColorBlendAttachmentInfo.dstColorBlendFactor = p_DstColor;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetColorBlendOperation(
    const VkBlendOp p_ColorOp) noexcept
{
    m_ColorBlendAttachmentInfo.colorBlendOp = p_ColorOp;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetAlphaBlendFactors(
    const VkBlendFactor p_SrcAlpha, const VkBlendFactor p_DstAlpha) noexcept
{
    m_ColorBlendAttachmentInfo.srcAlphaBlendFactor = p_SrcAlpha;
    m_ColorBlendAttachmentInfo.dstAlphaBlendFactor = p_DstAlpha;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetAlphaBlendOperation(
    const VkBlendOp p_AlphaOp) noexcept
{
    m_ColorBlendAttachmentInfo.alphaBlendOp = p_AlphaOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::ColorAttachmentBuilder::EndColorAttachment() noexcept
{
    m_Builder->m_ColorAttachments.push_back(m_ColorBlendAttachmentInfo);
    return *m_Builder;
}

} // namespace VKit