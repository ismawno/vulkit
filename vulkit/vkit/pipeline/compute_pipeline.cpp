#include "vkit/core/pch.hpp"
#include "vkit/pipeline/compute_pipeline.hpp"

namespace VKit
{
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
    pipelineInfo.stage.module = p_Specs.ComputeShader->GetModule();
    pipelineInfo.stage.pName = p_Specs.EntryPoint;

    return Result<VkComputePipelineCreateInfo>::Ok(pipelineInfo);
}

Result<ComputePipeline> ComputePipeline::Create(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs) noexcept
{
    const auto result = createPipelineInfo(p_Specs);
    if (!result)
        return Result<ComputePipeline>::Error(result.GetError());

    const VkComputePipelineCreateInfo &pipelineInfo = result.GetValue();
    VkPipeline pipeline;
    const VkResult result =
        vkCreateComputePipelines(p_Device, p_Specs.Cache, 1, &pipelineInfo, p_Device.AllocationCallbacks, &pipeline);
}
VulkanResult ComputePipeline::Create(const LogicalDevice::Proxy &p_Device, const std::span<const Specs> p_Specs,
                                     const std::span<ComputePipeline> p_Pipelines) noexcept
{
    if (p_Specs.size() != p_Pipelines.size())
        return VulkanResult::Error(VK_ERROR_INITIALIZATION_FAILED, "Specs and pipelines must have the same size");
    if (p_Specs.size() == 0)
        return VulkanResult::Error(VK_ERROR_INITIALIZATION_FAILED, "Specs and pipelines must not be empty");

    DynamicArray<VkComputePipelineCreateInfo> pipelineInfos;
    pipelineInfos.reserve(p_Specs.size());
    for (const Specs &specs : p_Specs)
    {
        const auto result = createPipelineInfo(specs);
        if (!result)
            return result.GetError();
        const VkComputePipelineCreateInfo &pipelineInfo = result.GetValue();
        pipelineInfos.push_back(pipelineInfo);
    }

    DynamicArray<VkPipeline> pipelines{p_Specs.size()};
    const VkResult result =
        vkCreateComputePipelines(p_Device, p_Specs[0].Cache, static_cast<u32>(p_Specs.size()), pipelineInfos.data(),
                                 p_Device.AllocationCallbacks, pipelines.data());
    if (result != VK_SUCCESS)
        return VulkanResult::Error(result, "Failed to create compute pipelines");

    for (usize i = 0; i < p_Specs.size(); ++i)
        p_Pipelines[i] = ComputePipeline(p_Device, pipelines[i], p_Specs[i].Layout, *p_Specs[i].ComputeShader);
    return VulkanResult::Success();
}

} // namespace VKit