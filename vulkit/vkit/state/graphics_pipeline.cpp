#include "vkit/core/pch.hpp"
#include "vkit/state/graphics_pipeline.hpp"
#include "tkit/utils/debug.hpp"

namespace VKit
{
GraphicsPipeline::Builder::Builder(const ProxyDevice &p_Device, const VkPipelineLayout p_Layout,
                                   const VkRenderPass p_RenderPass, const u32 p_Subpass)
    : m_Device(p_Device), m_Layout(p_Layout), m_RenderPass(p_RenderPass), m_Subpass(p_Subpass)
{
    initialize();
}
GraphicsPipeline::Builder::Builder(const ProxyDevice &p_Device, const VkPipelineLayout p_Layout,
                                   const VkPipelineRenderingCreateInfoKHR &p_RenderingInfo)
    : m_Device(p_Device), m_Layout(p_Layout), m_RenderPass(VK_NULL_HANDLE), m_RenderingInfo(p_RenderingInfo)
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
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCreateGraphicsPipelines, Result<GraphicsPipeline>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkDestroyPipeline, Result<GraphicsPipeline>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCmdBindPipeline, Result<GraphicsPipeline>);

    const VkGraphicsPipelineCreateInfo pipelineInfo = CreatePipelineInfo();

    VkPipeline pipeline;
    const VkResult result = m_Device.Table->CreateGraphicsPipelines(m_Device, m_Cache, 1, &pipelineInfo,
                                                                    m_Device.AllocationCallbacks, &pipeline);
    if (result != VK_SUCCESS)
        return Result<GraphicsPipeline>::Error(result);

    return Result<GraphicsPipeline>::Ok(m_Device, pipeline);
}

Result<> GraphicsPipeline::Create(const ProxyDevice &p_Device, const TKit::Span<Builder> p_Builders,
                                  const TKit::Span<GraphicsPipeline> p_Pipelines, const VkPipelineCache p_Cache)
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCreateGraphicsPipelines, Result<>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkDestroyPipeline, Result<>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCmdBindPipeline, Result<>);

    if (p_Builders.GetSize() != p_Pipelines.GetSize())
        return Result<>::Error(Error_BadInput, TKit::Format("Specs size ({}) and pipelines ({}) size must be equal",
                                                                p_Builders.GetSize(), p_Pipelines.GetSize()));
    if (p_Builders.GetSize() == 0)
        return Result<>::Error(Error_BadInput, "Specs and pipelines must not be empty");

    TKit::Array32<VkGraphicsPipelineCreateInfo> pipelineInfos;
    for (Builder &builder : p_Builders)
        pipelineInfos.Append(builder.CreatePipelineInfo());

    const u32 count = p_Builders.GetSize();
    TKit::Array32<VkPipeline> pipelines{count};
    const VkResult result = p_Device.Table->CreateGraphicsPipelines(p_Device, p_Cache, count, pipelineInfos.GetData(),
                                                                    p_Device.AllocationCallbacks, pipelines.GetData());

    if (result != VK_SUCCESS)
        return Result<>::Error(result);

    for (u32 i = 0; i < count; ++i)
        p_Pipelines[i] = GraphicsPipeline(p_Device, pipelines[i]);

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
void GraphicsPipeline::Bind(VkCommandBuffer p_CommandBuffer) const
{
    m_Device.Table->CmdBindPipeline(p_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
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

GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBasePipeline(const VkPipeline p_BasePipeline)
{
    m_BasePipeline = p_BasePipeline;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBasePipelineIndex(const i32 p_BasePipelineIndex)
{
    m_BasePipelineIndex = p_BasePipelineIndex;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetCache(const VkPipelineCache p_Cache)
{
    m_Cache = p_Cache;
    return *this;
}

// Input Assembly
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetTopology(const VkPrimitiveTopology p_Topology)
{
    m_InputAssemblyInfo.topology = p_Topology;
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
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddViewport(const VkViewport p_Viewport, const VkRect2D p_Scissor)
{
    m_Viewports.Append(std::make_pair(p_Viewport, p_Scissor));
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddViewports(
    const TKit::Span<std::pair<VkViewport, VkRect2D>> p_Viewports)
{
    for (const auto &viewport : p_Viewports)
        m_Viewports.Append(viewport);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetViewports(
    const TKit::Span<std::pair<VkViewport, VkRect2D>> p_Viewports)
{
    m_Viewports.Clear();
    AddViewports(p_Viewports);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetViewportCount(const u32 p_ViewportCount)
{
    m_ViewportInfo.viewportCount = p_ViewportCount;
    m_ViewportInfo.scissorCount = p_ViewportCount;
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
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetPolygonMode(const VkPolygonMode p_Mode)
{
    m_RasterizationInfo.polygonMode = p_Mode;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetLineWidth(const f32 p_Width)
{
    m_RasterizationInfo.lineWidth = p_Width;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetCullMode(const VkCullModeFlags p_Mode)
{
    m_RasterizationInfo.cullMode = p_Mode;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetFrontFace(const VkFrontFace p_FrontFace)
{
    m_RasterizationInfo.frontFace = p_FrontFace;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::EnableDepthBias()
{
    m_RasterizationInfo.depthBiasEnable = VK_TRUE;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetDepthBias(const f32 p_ConstantFactor, const f32 p_Clamp,
                                                                   const f32 p_SlopeFactor)
{
    m_RasterizationInfo.depthBiasConstantFactor = p_ConstantFactor;
    m_RasterizationInfo.depthBiasClamp = p_Clamp;
    m_RasterizationInfo.depthBiasSlopeFactor = p_SlopeFactor;
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
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetSampleCount(const VkSampleCountFlagBits p_SampleCount)
{
    m_MultisampleInfo.rasterizationSamples = p_SampleCount;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetMinSampleShading(const f32 p_MinSampleShading)
{
    m_MultisampleInfo.minSampleShading = p_MinSampleShading;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetSampleMask(const VkSampleMask *p_SampleMask)
{
    m_MultisampleInfo.pSampleMask = p_SampleMask;
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
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetLogicOperation(const VkLogicOp p_Operation)
{
    m_ColorBlendInfo.logicOp = p_Operation;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBlendConstants(const f32 *p_Constants)
{
    m_ColorBlendInfo.blendConstants[0] = p_Constants[0];
    m_ColorBlendInfo.blendConstants[1] = p_Constants[1];
    m_ColorBlendInfo.blendConstants[2] = p_Constants[2];
    m_ColorBlendInfo.blendConstants[3] = p_Constants[3];
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBlendConstants(const f32 p_C1, const f32 p_C2, const f32 p_C3,
                                                                        const f32 p_C4)
{
    m_ColorBlendInfo.blendConstants[0] = p_C1;
    m_ColorBlendInfo.blendConstants[1] = p_C2;
    m_ColorBlendInfo.blendConstants[2] = p_C3;
    m_ColorBlendInfo.blendConstants[3] = p_C4;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetBlendConstant(const u32 p_Index, const f32 p_Value)
{
    m_ColorBlendInfo.blendConstants[p_Index] = p_Value;
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
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetDepthCompareOperation(const VkCompareOp p_Op)
{
    m_DepthStencilInfo.depthCompareOp = p_Op;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetDepthBounds(const f32 p_Min, const f32 p_Max)
{
    m_DepthStencilInfo.minDepthBounds = p_Min;
    m_DepthStencilInfo.maxDepthBounds = p_Max;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilFailOperation(const VkStencilOp p_FailOp,
                                                                              const StencilOperationFlags p_Flags)
{
    if (p_Flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.failOp = p_FailOp;
    if (p_Flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.failOp = p_FailOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilPassOperation(const VkStencilOp p_PassOp,
                                                                              const StencilOperationFlags p_Flags)
{
    if (p_Flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.passOp = p_PassOp;
    if (p_Flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.passOp = p_PassOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilDepthFailOperation(const VkStencilOp p_DepthFailOp,
                                                                                   const StencilOperationFlags p_Flags)
{
    if (p_Flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.depthFailOp = p_DepthFailOp;
    if (p_Flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.depthFailOp = p_DepthFailOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilCompareOperation(const VkCompareOp p_CompareOp,
                                                                                 const StencilOperationFlags p_Flags)
{
    if (p_Flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.compareOp = p_CompareOp;
    if (p_Flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.compareOp = p_CompareOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilCompareMask(const u32 p_Mask,
                                                                            const StencilOperationFlags p_Flags)
{
    if (p_Flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.compareMask = p_Mask;
    if (p_Flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.compareMask = p_Mask;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilWriteMask(const u32 p_Mask,
                                                                          const StencilOperationFlags p_Flags)
{
    if (p_Flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.writeMask = p_Mask;
    if (p_Flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.writeMask = p_Mask;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetStencilReference(const u32 p_Reference,
                                                                          const StencilOperationFlags p_Flags)
{
    if (p_Flags & StencilOperationFlag_Front)
        m_DepthStencilInfo.front.reference = p_Reference;
    if (p_Flags & StencilOperationFlag_Back)
        m_DepthStencilInfo.back.reference = p_Reference;
    return *this;
}

// Vertex Input
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddBindingDescription(const VkVertexInputRate p_InputRate,
                                                                            const u32 p_Stride)
{
    VkVertexInputBindingDescription binding{};
    binding.binding = m_BindingDescriptions.GetSize();
    binding.stride = p_Stride;
    binding.inputRate = p_InputRate;
    m_BindingDescriptions.Append(binding);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddAttributeDescription(const u32 p_Binding,
                                                                              const VkFormat p_Format,
                                                                              const u32 p_Offset)
{
    VkVertexInputAttributeDescription attribute{};
    attribute.binding = p_Binding;
    attribute.format = p_Format;
    attribute.location = m_AttributeDescriptions.GetSize();
    attribute.offset = p_Offset;
    m_AttributeDescriptions.Append(attribute);
    return *this;
}

// Shader Stages
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddShaderStage(const VkShaderModule p_Module,
                                                                     const VkShaderStageFlagBits p_Stage,
                                                                     const VkPipelineShaderStageCreateFlags p_Flags,
                                                                     const VkSpecializationInfo *p_Info,
                                                                     const char *p_EntryPoint)
{
    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.module = p_Module;
    stage.stage = p_Stage;
    stage.flags = p_Flags;
    stage.pSpecializationInfo = p_Info;
    stage.pName = p_EntryPoint;
    m_ShaderStages.Append(stage);
    return *this;
}

// Dynamic State
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddDynamicState(const VkDynamicState p_State)
{
    m_DynamicStates.Append(p_State);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::AddDynamicStates(const TKit::Span<const VkDynamicState> p_States)
{
    for (const VkDynamicState state : p_States)
        m_DynamicStates.Append(state);
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::Builder::SetDynamicStates(const TKit::Span<const VkDynamicState> p_States)
{
    m_DynamicStates.Clear();
    AddDynamicStates(p_States);
    return *this;
}

GraphicsPipeline::ColorAttachmentBuilder::ColorAttachmentBuilder(Builder *p_Builder) : m_Builder(p_Builder)
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
    const VkColorComponentFlags p_WriteMask)
{
    m_ColorBlendAttachmentInfo.colorWriteMask = p_WriteMask;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetColorBlendFactors(
    const VkBlendFactor p_SrcColor, const VkBlendFactor p_DstColor)
{
    m_ColorBlendAttachmentInfo.srcColorBlendFactor = p_SrcColor;
    m_ColorBlendAttachmentInfo.dstColorBlendFactor = p_DstColor;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetColorBlendOperation(
    const VkBlendOp p_ColorOp)
{
    m_ColorBlendAttachmentInfo.colorBlendOp = p_ColorOp;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetAlphaBlendFactors(
    const VkBlendFactor p_SrcAlpha, const VkBlendFactor p_DstAlpha)
{
    m_ColorBlendAttachmentInfo.srcAlphaBlendFactor = p_SrcAlpha;
    m_ColorBlendAttachmentInfo.dstAlphaBlendFactor = p_DstAlpha;
    return *this;
}
GraphicsPipeline::ColorAttachmentBuilder &GraphicsPipeline::ColorAttachmentBuilder::SetAlphaBlendOperation(
    const VkBlendOp p_AlphaOp)
{
    m_ColorBlendAttachmentInfo.alphaBlendOp = p_AlphaOp;
    return *this;
}
GraphicsPipeline::Builder &GraphicsPipeline::ColorAttachmentBuilder::EndColorAttachment()
{
    m_Builder->m_ColorAttachments.Append(m_ColorBlendAttachmentInfo);
    return *m_Builder;
}

} // namespace VKit
