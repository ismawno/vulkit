#include "vkit/core/pch.hpp"
#include "vkit/state/compute_pipeline.hpp"
#include "tkit/container/stack_array.hpp"

namespace VKit
{

static VkComputePipelineCreateInfo createPipelineInfo(const ComputePipeline::Specs &p_Specs)
{
    TKIT_ASSERT(p_Specs.Layout, "[VULKIT][PIPELINE] Pipeline layout must be provided");
    TKIT_ASSERT(p_Specs.ComputeShader, "[VULKIT][PIPELINE] Compute shader must be provided");

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = p_Specs.Layout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = p_Specs.ComputeShader;
    pipelineInfo.stage.pName = p_Specs.EntryPoint;

    return pipelineInfo;
}

Result<ComputePipeline> ComputePipeline::Create(const ProxyDevice &p_Device, const Specs &p_Specs)
{
    const VkComputePipelineCreateInfo pipelineInfo = createPipelineInfo(p_Specs);

    VkPipeline pipeline;
    const VkResult result = p_Device.Table->CreateComputePipelines(p_Device, p_Specs.Cache, 1, &pipelineInfo,
                                                                   p_Device.AllocationCallbacks, &pipeline);
    if (result != VK_SUCCESS)
        return Result<ComputePipeline>::Error(result);

    return Result<ComputePipeline>::Ok(p_Device, pipeline);
}
Result<> ComputePipeline::Create(const ProxyDevice &p_Device, const TKit::Span<const Specs> p_Specs,
                                 const TKit::Span<ComputePipeline> p_Pipelines, const VkPipelineCache p_Cache)
{
    TKit::StackArray<VkComputePipelineCreateInfo> pipelineInfos{};
    pipelineInfos.Reserve(p_Specs.GetSize());
    for (const Specs &specs : p_Specs)
        pipelineInfos.Append(createPipelineInfo(specs));

    const u32 count = p_Specs.GetSize();
    TKit::StackArray<VkPipeline> pipelines{count};
    const VkResult result = p_Device.Table->CreateComputePipelines(p_Device, p_Cache, count, pipelineInfos.GetData(),
                                                                   p_Device.AllocationCallbacks, pipelines.GetData());

    if (result != VK_SUCCESS)
        return Result<>::Error(result);

    for (u32 i = 0; i < count; ++i)
        p_Pipelines[i] = ComputePipeline(p_Device, pipelines[i]);
    return Result<>::Ok();
}

void ComputePipeline::Destroy()
{
    if (m_Pipeline)
    {
        m_Device.Table->DestroyPipeline(m_Device, m_Pipeline, m_Device.AllocationCallbacks);
        m_Pipeline = VK_NULL_HANDLE;
    }
}

void ComputePipeline::Bind(VkCommandBuffer p_CommandBuffer) const
{
    m_Device.Table->CmdBindPipeline(p_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
}

} // namespace VKit
