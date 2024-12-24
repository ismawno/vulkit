#include "vkit/core/pch.hpp"
#include "vkit/pipeline/compute_pipeline.hpp"

namespace VKit
{
ComputePipeline::ComputePipeline(const LogicalDevice::Proxy &p_Device, VkPipeline p_Pipeline) noexcept
    : m_Device(p_Device), m_Pipeline(p_Pipeline)
{
}

static Result<VkComputePipelineCreateInfo> createPipelineInfo(const ComputePipeline::Specs &p_Specs) noexcept
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

Result<ComputePipeline> ComputePipeline::Create(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs) noexcept
{
    const auto presult = createPipelineInfo(p_Specs);
    if (!presult)
        return Result<ComputePipeline>::Error(presult.GetError());

    const VkComputePipelineCreateInfo &pipelineInfo = presult.GetValue();

    VkPipeline pipeline;
    const VkResult result =
        vkCreateComputePipelines(p_Device, p_Specs.Cache, 1, &pipelineInfo, p_Device.AllocationCallbacks, &pipeline);
    if (result != VK_SUCCESS)
        return Result<ComputePipeline>::Error(result, "Failed to create compute pipeline");

    return Result<ComputePipeline>::Ok(p_Device, pipeline);
}
VulkanResult ComputePipeline::Create(const LogicalDevice::Proxy &p_Device, const std::span<const Specs> p_Specs,
                                     const std::span<ComputePipeline> p_Pipelines,
                                     const VkPipelineCache p_Cache) noexcept
{
    TKit::StaticArray32<VkComputePipelineCreateInfo> pipelineInfos;
    for (const Specs &specs : p_Specs)
    {
        const auto result = createPipelineInfo(specs);
        if (!result)
            return result.GetError();
        pipelineInfos.push_back(result.GetValue());
    }

    TKit::StaticArray32<VkPipeline> pipelines{p_Specs.size()};
    const VkResult result =
        vkCreateComputePipelines(p_Device, p_Cache, static_cast<u32>(p_Specs.size()), pipelineInfos.data(),
                                 p_Device.AllocationCallbacks, pipelines.data());
    if (result != VK_SUCCESS)
        return VulkanResult::Error(result, "Failed to create compute pipelines");

    for (usize i = 0; i < p_Specs.size(); ++i)
        p_Pipelines[i] = ComputePipeline(p_Device, pipelines[i]);
    return VulkanResult::Success();
}

void ComputePipeline::Destroy() noexcept
{
    TKIT_ASSERT(m_Pipeline, "The compute pipeline is a NULL handle");
    vkDestroyPipeline(m_Device, m_Pipeline, m_Device.AllocationCallbacks);
    m_Pipeline = VK_NULL_HANDLE;
}

void ComputePipeline::SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
{
    const VkPipeline pipeline = m_Pipeline;
    const LogicalDevice::Proxy device = m_Device;
    p_Queue.Push([pipeline, device]() { vkDestroyPipeline(device, pipeline, device.AllocationCallbacks); });
}

void ComputePipeline::Bind(VkCommandBuffer p_CommandBuffer) const noexcept
{
    vkCmdBindPipeline(p_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
}

VkPipeline ComputePipeline::GetPipeline() const noexcept
{
    return m_Pipeline;
}
ComputePipeline::operator VkPipeline() const noexcept
{
    return m_Pipeline;
}
ComputePipeline::operator bool() const noexcept
{
    return m_Pipeline != VK_NULL_HANDLE;
}

} // namespace VKit