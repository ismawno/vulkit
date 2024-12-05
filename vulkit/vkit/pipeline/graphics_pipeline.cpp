#include "vkit/core/pch.hpp"
#include "vkit/pipeline/graphics_pipeline.hpp"
#include "tkit/core/logging.hpp"
#include "tkit/memory/stack_allocator.hpp"

#include <filesystem>

namespace VKit
{
Result<GraphicsPipeline> GraphicsPipeline::Create(const LogicalDevice::Proxy &p_Device, Specs p_Specs) noexcept
{
    p_Specs.Populate();
    TKIT_ASSERT(p_Specs.RenderPass, "Render pass must be provided to create graphics pipeline");
    TKIT_ASSERT(p_Specs.Layout, "Pipeline layout must be provided to create graphics pipeline");

    auto shaderResult = Shader::Create(p_Device, p_Specs.VertexShaderBinaryPath);
    if (!shaderResult)
        return Result<GraphicsPipeline>::Error(shaderResult.GetError());
    const Shader vertexShader = shaderResult.GetValue();

    shaderResult = Shader::Create(p_Device, p_Specs.FragmentShaderBinaryPath);
    if (!shaderResult)
        return Result<GraphicsPipeline>::Error(shaderResult.GetError());
    const Shader fragmentShader = shaderResult.GetValue();

    const bool hasAttributes = !p_Specs.AttributeDescriptions.empty();
    const bool hasBindings = !p_Specs.BindingDescriptions.empty();

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
    for (auto &shaderStage : shaderStages)
    {
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.pName = "main";
        shaderStage.flags = 0;
        shaderStage.pNext = nullptr;
        shaderStage.pSpecializationInfo = nullptr;
    }
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShader;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentShader;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    if (hasAttributes || hasBindings)
    {
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(p_Specs.AttributeDescriptions.size());
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<u32>(p_Specs.BindingDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = hasAttributes ? p_Specs.AttributeDescriptions.data() : nullptr;
        vertexInputInfo.pVertexBindingDescriptions = hasBindings ? p_Specs.BindingDescriptions.data() : nullptr;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &p_Specs.InputAssemblyInfo;
    pipelineInfo.pViewportState = &p_Specs.ViewportInfo;
    pipelineInfo.pRasterizationState = &p_Specs.RasterizationInfo;
    pipelineInfo.pMultisampleState = &p_Specs.MultisampleInfo;
    pipelineInfo.pColorBlendState = &p_Specs.ColorBlendInfo;
    pipelineInfo.pDepthStencilState = &p_Specs.DepthStencilInfo;
    pipelineInfo.pDynamicState = &p_Specs.DynamicStateInfo;

    pipelineInfo.layout = p_Specs.Layout;
    pipelineInfo.renderPass = p_Specs.RenderPass;
    pipelineInfo.subpass = p_Specs.Subpass;

    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline;
    const VkResult result =
        vkCreateGraphicsPipelines(p_Device, p_Specs.Cache, 1, &pipelineInfo, p_Device.AllocationCallbacks, &pipeline);
    if (result != VK_SUCCESS)
        return Result<GraphicsPipeline>::Error(result, "Failed to create graphics pipeline");

    return Result<GraphicsPipeline>::Ok(p_Device, pipeline, p_Specs.Layout, vertexShader, fragmentShader);
}

GraphicsPipeline::GraphicsPipeline(const LogicalDevice::Proxy &p_Device, const VkPipeline p_Pipeline,
                                   const VkPipelineLayout p_PipelineLayout, const Shader &p_VertexShader,
                                   const Shader &p_FragmentShader) noexcept
    : m_Device(p_Device), m_Pipeline(p_Pipeline), m_Layout(p_PipelineLayout), m_VertexShader(p_VertexShader),
      m_FragmentShader(p_FragmentShader)
{
}

void GraphicsPipeline::Destroy() noexcept
{
    TKIT_ASSERT(m_Pipeline, "The graphics pipeline is already destroyed");
    m_VertexShader.Destroy();
    m_FragmentShader.Destroy();
    vkDestroyPipeline(m_Device, m_Pipeline, m_Device.AllocationCallbacks);
    m_Pipeline = VK_NULL_HANDLE;
}
void GraphicsPipeline::SubmitForDeletion(DeletionQueue &p_Queue) noexcept
{
    const VkPipeline pipeline = m_Pipeline;
    const LogicalDevice::Proxy device = m_Device;
    m_VertexShader.SubmitForDeletion(p_Queue);
    m_FragmentShader.SubmitForDeletion(p_Queue);
    p_Queue.Push([pipeline, device]() { vkDestroyPipeline(device, pipeline, device.AllocationCallbacks); });
}

void GraphicsPipeline::Bind(VkCommandBuffer p_CommandBuffer) const noexcept
{
    vkCmdBindPipeline(p_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
}

VkPipelineLayout GraphicsPipeline::GetLayout() const noexcept
{
    return m_Layout;
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

GraphicsPipeline::Specs::Specs() noexcept
{
    InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportInfo.viewportCount = 1;
    ViewportInfo.pViewports = nullptr;
    ViewportInfo.scissorCount = 1;
    ViewportInfo.pScissors = nullptr;

    RasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizationInfo.depthClampEnable = VK_FALSE;
    RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    RasterizationInfo.lineWidth = 1.0f;
    RasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    RasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    RasterizationInfo.depthBiasEnable = VK_FALSE;
    RasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
    RasterizationInfo.depthBiasClamp = 0.0f;          // Optional
    RasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

    MultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisampleInfo.sampleShadingEnable = VK_FALSE;
    MultisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    MultisampleInfo.minSampleShading = 1.0f;          // Optional
    MultisampleInfo.pSampleMask = nullptr;            // Optional
    MultisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional
    MultisampleInfo.alphaToOneEnable = VK_FALSE;      // Optional

    ColorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_TRUE;
    ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;           // Optional
    ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
    ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;                            // Optional
    ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;                 // Optional
    ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
    ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;                            // Optional

    ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendInfo.logicOpEnable = VK_FALSE;
    ColorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    ColorBlendInfo.attachmentCount = 1;
    ColorBlendInfo.blendConstants[0] = 0.0f; // Optional
    ColorBlendInfo.blendConstants[1] = 0.0f; // Optional
    ColorBlendInfo.blendConstants[2] = 0.0f; // Optional
    ColorBlendInfo.blendConstants[3] = 0.0f; // Optional

    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_TRUE;
    DepthStencilInfo.depthWriteEnable = VK_TRUE;
    DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.minDepthBounds = 0.0f; // Optional
    DepthStencilInfo.maxDepthBounds = 1.0f; // Optional
    DepthStencilInfo.stencilTestEnable = VK_FALSE;
    DepthStencilInfo.front = {}; // Optional
    DepthStencilInfo.back = {};  // Optional

    DynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
}

void GraphicsPipeline::Specs::Populate() noexcept
{
    ColorBlendInfo.pAttachments = &ColorBlendAttachment;
    DynamicStateInfo.pDynamicStates = DynamicStates.data();
    DynamicStateInfo.dynamicStateCount = static_cast<u32>(DynamicStates.size());
}
} // namespace VKit