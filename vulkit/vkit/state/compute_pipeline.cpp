#include "vkit/core/pch.hpp"
#include "vkit/state/compute_pipeline.hpp"
#include "tkit/container/stack_array.hpp"

namespace VKit
{

static VkComputePipelineCreateInfo createPipelineInfo(const ComputePipeline::Specs &specs)
{
    TKIT_ASSERT(specs.Layout, "[VULKIT][PIPELINE] Pipeline layout must be provided");
    TKIT_ASSERT(specs.ComputeShader, "[VULKIT][PIPELINE] Compute shader must be provided");

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = specs.Layout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = specs.ComputeShader;
    pipelineInfo.stage.pName = specs.EntryPoint;

    return pipelineInfo;
}

Result<ComputePipeline> ComputePipeline::Create(const ProxyDevice &device, const Specs &specs)
{
    const VkComputePipelineCreateInfo pipelineInfo = createPipelineInfo(specs);

    VkPipeline pipeline;
    VKIT_RETURN_IF_FAILED(device.Table->CreateComputePipelines(device, specs.Cache, 1, &pipelineInfo,
                                                               device.AllocationCallbacks, &pipeline),
                          Result<ComputePipeline>);

    return Result<ComputePipeline>::Ok(device, pipeline);
}
Result<> ComputePipeline::Create(const ProxyDevice &device, const TKit::Span<const Specs> specs,
                                 const TKit::Span<ComputePipeline> pipelines, const VkPipelineCache cache)
{
    TKit::StackArray<VkComputePipelineCreateInfo> pipelineInfos{};
    pipelineInfos.Reserve(specs.GetSize());
    for (const Specs &specs : specs)
        pipelineInfos.Append(createPipelineInfo(specs));

    const u32 count = specs.GetSize();
    TKit::StackArray<VkPipeline> vkpipelines{count};

    VKIT_RETURN_IF_FAILED(device.Table->CreateComputePipelines(device, cache, count, pipelineInfos.GetData(),
                                                               device.AllocationCallbacks, vkpipelines.GetData()),
                          Result<>);

    for (u32 i = 0; i < count; ++i)
        pipelines[i] = ComputePipeline(device, vkpipelines[i]);
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

void ComputePipeline::Bind(VkCommandBuffer commandBuffer) const
{
    m_Device.Table->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
}

} // namespace VKit
