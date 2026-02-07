#include "vkit/core/pch.hpp"
#include "vkit/state/graphics_pipeline.hpp"
#include "tkit/container/stack_array.hpp"

namespace VKit
{
GraphicsPipeline::Builder::Builder(const ProxyDevice &device, const VkPipelineLayout layout,
                                   const VkRenderPass renderPass, const u32 subpass)
    : m_Device(device), m_Layout(layout), m_RenderPass(renderPass), m_Subpass(subpass)
{
    initialize();
}
GraphicsPipeline::Builder::Builder(const ProxyDevice &device, const VkPipelineLayout layout,
                                   const VkPipelineRenderingCreateInfoKHR &renderingInfo)
    : m_Device(device), m_Layout(layout), m_RenderPass(VK_NULL_HANDLE), m_RenderingInfo(renderingInfo)
{
    initialize();
}

void GraphicsPipeline::Builder::initialize()
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

Result<GraphicsPipeline> GraphicsPipeline::Builder::Build() const
{
    const VkGraphicsPipelineCreateInfo pipelineInfo = CreatePipelineInfo();

    VkPipeline pipeline;
    VKIT_RETURN_IF_FAILED(m_Device.Table->CreateGraphicsPipelines(m_Device, m_Cache, 1, &pipelineInfo,
                                                                  m_Device.AllocationCallbacks, &pipeline),
                          Result<GraphicsPipeline>);

    return Result<GraphicsPipeline>::Ok(m_Device, pipeline);
}

Result<> GraphicsPipeline::Create(const ProxyDevice &device, const TKit::Span<const Builder> builders,
                                  const TKit::Span<GraphicsPipeline> pipelines, const VkPipelineCache cache)
{
    TKIT_ASSERT(builders.GetSize() == pipelines.GetSize(),
                "[VULKIT][PIPELINE] Specs size ({}) and pipelines ({}) size must be equal", builders.GetSize(),
                pipelines.GetSize());
    TKIT_ASSERT(!builders.IsEmpty(), "[VULKIT][PIPELINE] Specs and pipelines must not be empty");

    TKit::StackArray<VkGraphicsPipelineCreateInfo> pipelineInfos;
    pipelineInfos.Reserve(builders.GetSize());
    for (const Builder &builder : builders)
        pipelineInfos.Append(builder.CreatePipelineInfo());

    const u32 count = builders.GetSize();
    TKit::StackArray<VkPipeline> vkpipelines{count};

    VKIT_RETURN_IF_FAILED(device.Table->CreateGraphicsPipelines(device, cache, count, pipelineInfos.GetData(),
                                                                device.AllocationCallbacks, vkpipelines.GetData()),
                          Result<>);

    for (u32 i = 0; i < count; ++i)
        pipelines[i] = GraphicsPipeline(device, vkpipelines[i]);

    return Result<>::Ok();
}

void GraphicsPipeline::Destroy()
{
    if (m_Pipeline)
    {
        m_Device.Table->DestroyPipeline(m_Device, m_Pipeline, m_Device.AllocationCallbacks);
        m_Pipeline = VK_NULL_HANDLE;
    }
}
void GraphicsPipeline::Bind(VkCommandBuffer commandBuffer) const
{
    m_Device.Table->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
}

GraphicsPipeline::Builder &GraphicsPipeline::Builder::Bake()
{
    m_ColorBlendInfo.attachmentCount = m_ColorAttachments.GetSize();
    m_ColorBlendInfo.pAttachments = m_ColorAttachments.IsEmpty() ? nullptr : m_ColorAttachments.GetData();

    m_DynamicStateInfo.dynamicStateCount = m_DynamicStates.GetSize();
    m_DynamicStateInfo.pDynamicStates = m_DynamicStates.IsEmpty() ? nullptr : m_DynamicStates.GetData();

    m_VertexInputInfo.vertexAttributeDescriptionCount = m_AttributeDescriptions.GetSize();
    m_VertexInputInfo.vertexBindingDescriptionCount = m_BindingDescriptions.GetSize();
    m_VertexInputInfo.pVertexAttributeDescriptions =
        m_AttributeDescriptions.IsEmpty() ? nullptr : m_AttributeDescriptions.GetData();
    m_VertexInputInfo.pVertexBindingDescriptions =
        m_BindingDescriptions.IsEmpty() ? nullptr : m_BindingDescriptions.GetData();
    return *this;
}

VkGraphicsPipelineCreateInfo GraphicsPipeline::Builder::CreatePipelineInfo() const
{
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = m_ShaderStages.GetSize();
    pipelineInfo.pStages = m_ShaderStages.IsEmpty() ? nullptr : m_ShaderStages.GetData();
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
    pipelineInfo.pNext = m_RenderPass ? nullptr : &m_RenderingInfo;

    return pipelineInfo;
}

GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBasePipeline(const VkPipeline basePipeline)
{
    m_BasePipeline = basePipeline;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBasePipelineIndex(const i32 basePipelineIndex)
{
    m_BasePipelineIndex = basePipelineIndex;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetCache(const VkPipelineCache cache)
{
    m_Cache = cache;
    return *this;
}

// Input Assembly
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetTopology(const VkPrimitiveTopology topology)
{
    m_InputAssemblyInfo.topology = topology;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnablePrimitiveRestart()
{
    m_InputAssemblyInfo.primitiveRestartEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisablePrimitiveRestart()
{
    m_InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
    return *this;
}

// Viewport and Scissor
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddViewport(const VkViewport viewport, const VkRect2D scissor)
{
    m_Viewports.Append(viewport, scissor);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetViewportCount(const u32 viewportCount)
{
    m_ViewportInfo.viewportCount = viewportCount;
    m_ViewportInfo.scissorCount = viewportCount;
    m_ViewportInfo.pViewports = nullptr;
    m_ViewportInfo.pScissors = nullptr;
    return *this;
}

// Rasterization
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableRasterizerDiscard()
{
    m_RasterizationInfo.rasterizerDiscardEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableDepthClamp()
{
    m_RasterizationInfo.depthClampEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableRasterizerDiscard()
{
    m_RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableDepthClamp()
{
    m_RasterizationInfo.depthClampEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableDepthBias()
{
    m_RasterizationInfo.depthBiasEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetPolygonMode(const VkPolygonMode mode)
{
    m_RasterizationInfo.polygonMode = mode;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetLineWidth(const f32 width)
{
    m_RasterizationInfo.lineWidth = width;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetCullMode(const VkCullModeFlags mode)
{
    m_RasterizationInfo.cullMode = mode;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetFrontFace(const VkFrontFace frontFace)
{
    m_RasterizationInfo.frontFace = frontFace;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableDepthBias()
{
    m_RasterizationInfo.depthBiasEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetDepthBias(const f32 constantFactor, const f32 clamp,
                                                                   const f32 slopeFactor)
{
    m_RasterizationInfo.depthBiasConstantFactor = constantFactor;
    m_RasterizationInfo.depthBiasClamp = clamp;
    m_RasterizationInfo.depthBiasSlopeFactor = slopeFactor;
    return *this;
}

// Multisampling
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableSampleShading()
{
    m_MultisampleInfo.sampleShadingEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableSampleShading()
{
    m_MultisampleInfo.sampleShadingEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetSampleCount(const VkSampleCountFlagBits sampleCount)
{
    m_MultisampleInfo.rasterizationSamples = sampleCount;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetMinSampleShading(const f32 minSampleShading)
{
    m_MultisampleInfo.minSampleShading = minSampleShading;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetSampleMask(const VkSampleMask *sampleMask)
{
    m_MultisampleInfo.pSampleMask = sampleMask;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableAlphaToCoverage()
{
    m_MultisampleInfo.alphaToCoverageEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableAlphaToOne()
{
    m_MultisampleInfo.alphaToOneEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableAlphaToCoverage()
{
    m_MultisampleInfo.alphaToCoverageEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableAlphaToOne()
{
    m_MultisampleInfo.alphaToOneEnable = VK_FALSE;
    return *this;
}

// Color Blending
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableLogicOperation()
{
    m_ColorBlendInfo.logicOpEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableLogicOperation()
{
    m_ColorBlendInfo.logicOpEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetLogicOperation(const VkLogicOp operation)
{
    m_ColorBlendInfo.logicOp = operation;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBlendConstants(const f32 *constants)
{
    m_ColorBlendInfo.blendConstants[0] = constants[0];
    m_ColorBlendInfo.blendConstants[1] = constants[1];
    m_ColorBlendInfo.blendConstants[2] = constants[2];
    m_ColorBlendInfo.blendConstants[3] = constants[3];
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBlendConstants(const f32 c1, const f32 c2, const f32 c3,
                                                                        const f32 c4)
{
    m_ColorBlendInfo.blendConstants[0] = c1;
    m_ColorBlendInfo.blendConstants[1] = c2;
    m_ColorBlendInfo.blendConstants[2] = c3;
    m_ColorBlendInfo.blendConstants[3] = c4;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBlendConstant(const u32 index, const f32 value)
{
    m_ColorBlendInfo.blendConstants[index] = value;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddDefaultColorAttachment()
{
    m_ColorAttachmentBuilders.Append(this);
    m_ColorAttachments.Append(m_ColorAttachmentBuilders.GetBack().m_ColorBlendAttachmentInfo);
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::Builder::BeginColorAttachment()
{
    return m_ColorAttachmentBuilders.Append(this);
}

// Depth and Stencil
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableDepthTest()
{
    m_DepthStencilInfo.depthTestEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableDepthWrite()
{
    m_DepthStencilInfo.depthWriteEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableDepthBoundsTest()
{
    m_DepthStencilInfo.depthBoundsTestEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableStencilTest()
{
    m_DepthStencilInfo.stencilTestEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableDepthTest()
{
    m_DepthStencilInfo.depthTestEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableDepthWrite()
{
    m_DepthStencilInfo.depthWriteEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableDepthBoundsTest()
{
    m_DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::DisableStencilTest()
{
    m_DepthStencilInfo.stencilTestEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetDepthCompareOperation(const VkCompareOp op)
{
    m_DepthStencilInfo.depthCompareOp = op;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetDepthBounds(const f32 min, const f32 max)
{
    m_DepthStencilInfo.minDepthBounds = min;
    m_DepthStencilInfo.maxDepthBounds = max;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilFailOperation(const VkStencilOp failOp,
                                                                              const StencilOperationFlags flags)
{
    if (flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.failOp = failOp;
    if (flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.failOp = failOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilPassOperation(const VkStencilOp passOp,
                                                                              const StencilOperationFlags flags)
{
    if (flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.passOp = passOp;
    if (flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.passOp = passOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilDepthFailOperation(const VkStencilOp depthFailOp,
                                                                                   const StencilOperationFlags flags)
{
    if (flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.depthFailOp = depthFailOp;
    if (flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.depthFailOp = depthFailOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilCompareOperation(const VkCompareOp compareOp,
                                                                                 const StencilOperationFlags flags)
{
    if (flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.compareOp = compareOp;
    if (flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.compareOp = compareOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilCompareMask(const u32 mask,
                                                                            const StencilOperationFlags flags)
{
    if (flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.compareMask = mask;
    if (flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.compareMask = mask;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilWriteMask(const u32 mask,
                                                                          const StencilOperationFlags flags)
{
    if (flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.writeMask = mask;
    if (flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.writeMask = mask;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilReference(const u32 reference,
                                                                          const StencilOperationFlags flags)
{
    if (flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.reference = reference;
    if (flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.reference = reference;
    return *this;
}

// Vertex Input
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddBindingDescription(const u32 stride,
                                                                            const VkVertexInputRate inputRate)
{
    VkVertexInputBindingDescription binding{};
    binding.binding = m_BindingDescriptions.GetSize();
    binding.stride = stride;
    binding.inputRate = inputRate;
    m_BindingDescriptions.Append(binding);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddAttributeDescription(const u32 binding, const VkFormat format,
                                                                              const u32 offset)
{
    VkVertexInputAttributeDescription attribute{};
    attribute.binding = binding;
    attribute.format = format;
    attribute.location = m_AttributeDescriptions.GetSize();
    attribute.offset = offset;
    m_AttributeDescriptions.Append(attribute);
    return *this;
}

// Shader Stages
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddShaderStage(const VkShaderModule module,
                                                                     const VkShaderStageFlagBits pstage,
                                                                     const VkPipelineShaderStageCreateFlags flags,
                                                                     const VkSpecializationInfo *info,
                                                                     const char *entryPoint)
{
    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.module = module;
    stage.stage = pstage;
    stage.flags = flags;
    stage.pSpecializationInfo = info;
    stage.pName = entryPoint;
    m_ShaderStages.Append(stage);
    return *this;
}

// Dynamic State
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddDynamicState(const VkDynamicState state)
{
    m_DynamicStates.Append(state);
    return *this;
}

GraphicsPipeline::ColorAttachmentBuilder::ColorAttachmentBuilder(Builder *builder) : m_Builder(builder)
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

GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::EnableBlending()
{
    m_ColorBlendAttachmentInfo.blendEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::DisableBlending()
{
    m_ColorBlendAttachmentInfo.blendEnable = VK_FALSE;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetColorWriteMask(
    const VkColorComponentFlags writeMask)
{
    m_ColorBlendAttachmentInfo.colorWriteMask = writeMask;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetColorBlendFactors(
    const VkBlendFactor srcColor, const VkBlendFactor dstColor)
{
    m_ColorBlendAttachmentInfo.srcColorBlendFactor = srcColor;
    m_ColorBlendAttachmentInfo.dstColorBlendFactor = dstColor;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetColorBlendOperation(
    const VkBlendOp colorOp)
{
    m_ColorBlendAttachmentInfo.colorBlendOp = colorOp;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetAlphaBlendFactors(
    const VkBlendFactor srcAlpha, const VkBlendFactor dstAlpha)
{
    m_ColorBlendAttachmentInfo.srcAlphaBlendFactor = srcAlpha;
    m_ColorBlendAttachmentInfo.dstAlphaBlendFactor = dstAlpha;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetAlphaBlendOperation(
    const VkBlendOp alphaOp)
{
    m_ColorBlendAttachmentInfo.alphaBlendOp = alphaOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::ColorAttachmentBuilder::EndColorAttachment()
{
    m_Builder->m_ColorAttachments.Append(m_ColorBlendAttachmentInfo);
    return *m_Builder;
}

} // namespace VKit
