#include "vkit/core/pch.hpp"
#include "vkit/pipeline/compute_pipeline.hpp"

namespace VKit
{
ComputePipeline::ComputePipeline(const LogicalDevice::Proxy &p_Device, VkPipeline p_Pipeline)
    : m_Device(p_Device), m_Pipeline(p_Pipeline)
{
}

static Result<VkComputePipelineCreateInfo> createPipelineInfo(const ComputePipeline::Specs &p_Specs)
{
    if (!p_Specs.Layout)
        return Result<VkComputePipelineCreateInfo>::Error(VK_ERROR_INITIALIZATION_FAILED,
                                                          "Pipeline layout must be provided");
    if (!p_Specs.ComputeShader)
        return Result<VkComputePipelineCreateInfo>::Error(VK_ERROR_INITIALIZATION_FAILED,
                                                          "Compute shader must be provided");

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = p_Specs.Layout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = p_Specs.ComputeShader;
    pipelineInfo.stage.pName = p_Specs.EntryPoint;

    return Result<VkComputePipelineCreateInfo>::Ok(pipelineInfo);
}

Result<ComputePipeline> ComputePipeline::Create(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs)
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCreateComputePipelines, Result<ComputePipeline>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkDestroyPipeline, Result<ComputePipeline>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCmdBindPipeline, Result<ComputePipeline>);

    const auto presult = createPipelineInfo(p_Specs);
    if (!presult)
        return Result<ComputePipeline>::Error(presult.GetError());

    const VkComputePipelineCreateInfo &pipelineInfo = presult.GetValue();

    VkPipeline pipeline;
    const VkResult result = p_Device.Table->CreateComputePipelines(p_Device, p_Specs.Cache, 1, &pipelineInfo,
                                                                   p_Device.AllocationCallbacks, &pipeline);
    if (result != VK_SUCCESS)
        return Result<ComputePipeline>::Error(result, "Failed to create compute pipeline");

    return Result<ComputePipeline>::Ok(p_Device, pipeline);
}
Result<> ComputePipeline::Create(const LogicalDevice::Proxy &p_Device, const TKit::Span<const Specs> p_Specs,
                                 const TKit::Span<ComputePipeline> p_Pipelines, const VkPipelineCache p_Cache)
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCreateComputePipelines, Result<>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkDestroyPipeline, Result<>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCmdBindPipeline, Result<>);

    TKit::StaticArray32<VkComputePipelineCreateInfo> pipelineInfos;
    for (const Specs &specs : p_Specs)
    {
        const auto result = createPipelineInfo(specs);
        if (!result)
            return Result<>::Error(result.GetError());
        pipelineInfos.Append(result.GetValue());
    }

    const u32 count = p_Specs.GetSize();
    TKit::StaticArray32<VkPipeline> pipelines{count};
    const VkResult result = p_Device.Table->CreateComputePipelines(p_Device, p_Cache, count, pipelineInfos.GetData(),
                                                                   p_Device.AllocationCallbacks, pipelines.GetData());

    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to create compute pipelines");

    for (u32 i = 0; i < count; ++i)
        p_Pipelines[i] = ComputePipeline(p_Device, pipelines[i]);
    return Result<>::Ok();
}

void ComputePipeline::Destroy()
{
    TKIT_ASSERT(m_Pipeline, "[VULKIT] The compute pipeline is a NULL handle");
    m_Device.Table->DestroyPipeline(m_Device, m_Pipeline, m_Device.AllocationCallbacks);
    m_Pipeline = VK_NULL_HANDLE;
}

void ComputePipeline::SubmitForDeletion(DeletionQueue &p_Queue) const
{
    const VkPipeline pipeline = m_Pipeline;
    const LogicalDevice::Proxy device = m_Device;
    p_Queue.Push([pipeline, device]() { device.Table->DestroyPipeline(device, pipeline, device.AllocationCallbacks); });
}

void ComputePipeline::Bind(VkCommandBuffer p_CommandBuffer) const
{
    m_Device.Table->CmdBindPipeline(p_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
}

const LogicalDevice::Proxy &ComputePipeline::GetDevice() const
{
    return m_Device;
}
VkPipeline ComputePipeline::GetHandle() const
{
    return m_Pipeline;
}
ComputePipeline::operator VkPipeline() const
{
    return m_Pipeline;
}
ComputePipeline::operator bool() const
{
    return m_Pipeline != VK_NULL_HANDLE;
}

} // namespace VKit