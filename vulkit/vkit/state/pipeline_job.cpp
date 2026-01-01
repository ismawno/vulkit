#include "vkit/core/pch.hpp"
#include "vkit/state/pipeline_job.hpp"
#include "vkit/state/descriptor_set.hpp"

namespace VKit::Detail
{
template <Pipeline Pip>
IPipelineJob<Pip>::IPipelineJob(const Pip &p_Pipeline, const PipelineLayout &p_Layout)
    : m_Pipeline(p_Pipeline), m_Layout(p_Layout)
{
    m_DescriptorSets.Resize(p_Layout.GetInfo().DescriptorSetLayouts.GetSize(), VK_NULL_HANDLE);
    m_PushData.Resize(p_Layout.GetInfo().PushConstantRanges.GetSize());
}

template <Pipeline Pip>
void IPipelineJob<Pip>::Bind(const VkCommandBuffer p_CommandBuffer, u32 p_FirstSet,
                             const TKit::Span<const u32> p_DynamicOffsets) const
{
    m_Pipeline.Bind(p_CommandBuffer);
    u32 offset = 0;

    const LogicalDevice::Proxy &device = m_Pipeline.GetDevice();
    // Data may not need to be pushed every frame... but I guess it is a small price to pay for the flexibility
    for (u32 i = 0; i < m_PushData.GetSize(); ++i)
    {
        const PushDataInfo &info = m_PushData[i];
        if (!info.Data)
            continue;
        device.Table->CmdPushConstants(p_CommandBuffer, m_Layout, info.Stages, offset, info.Size, info.Data);
        offset += info.Size;
    }

    TKit::Array8<VkDescriptorSet> descriptorSets;
    for (const VkDescriptorSet set : m_DescriptorSets)
        if (set)
            descriptorSets.Append(set);
    if (descriptorSets.IsEmpty())
        return;

    if constexpr (std::is_same_v<Pip, GraphicsPipeline>)
        DescriptorSet::Bind(device, p_CommandBuffer, m_DescriptorSets, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Layout,
                            p_FirstSet, p_DynamicOffsets);
    else
        DescriptorSet::Bind(device, p_CommandBuffer, m_DescriptorSets, VK_PIPELINE_BIND_POINT_COMPUTE, m_Layout,
                            p_FirstSet, p_DynamicOffsets);
}
template <Pipeline Pip> IPipelineJob<Pip>::operator bool() const
{
    return m_Pipeline;
}

Result<PipelineJob<GraphicsPipeline>> PipelineJob<GraphicsPipeline>::Create(const GraphicsPipeline &p_Pipeline,
                                                                            const PipelineLayout &p_Layout)
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Pipeline.GetDevice().Table, vkCmdPushConstants,
                                        Result<PipelineJob<GraphicsPipeline>>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Pipeline.GetDevice().Table, vkCmdBindDescriptorSets,
                                        Result<PipelineJob<GraphicsPipeline>>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Pipeline.GetDevice().Table, vkCmdDraw, Result<PipelineJob<GraphicsPipeline>>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Pipeline.GetDevice().Table, vkCmdDrawIndexed,
                                        Result<PipelineJob<GraphicsPipeline>>);
    return Result<PipelineJob<GraphicsPipeline>>::Ok(p_Pipeline, p_Layout);
}

void PipelineJob<GraphicsPipeline>::Draw(const VkCommandBuffer p_CommandBuffer, const u32 p_VertexCount,
                                         const u32 p_InstanceCount, const u32 p_FirstVertex,
                                         const u32 p_FirstInstance) const
{
    const LogicalDevice::Proxy &device = m_Pipeline.GetDevice();
    device.Table->CmdDraw(p_CommandBuffer, p_VertexCount, p_InstanceCount, p_FirstVertex, p_FirstInstance);
}
void PipelineJob<GraphicsPipeline>::DrawIndexed(const VkCommandBuffer p_CommandBuffer, const u32 p_IndexCount,
                                                const u32 p_InstanceCount, const u32 p_FirstIndex,
                                                const i32 p_VertexOffset, const u32 p_FirstInstance) const
{
    const LogicalDevice::Proxy &device = m_Pipeline.GetDevice();
    device.Table->CmdDrawIndexed(p_CommandBuffer, p_IndexCount, p_InstanceCount, p_FirstIndex, p_VertexOffset,
                                 p_FirstInstance);
}

Result<PipelineJob<ComputePipeline>> PipelineJob<ComputePipeline>::Create(const ComputePipeline &p_Pipeline,
                                                                          const PipelineLayout &p_Layout)
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Pipeline.GetDevice().Table, vkCmdPushConstants,
                                        Result<PipelineJob<ComputePipeline>>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Pipeline.GetDevice().Table, vkCmdBindDescriptorSets,
                                        Result<PipelineJob<ComputePipeline>>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Pipeline.GetDevice().Table, vkCmdDispatch,
                                        Result<PipelineJob<ComputePipeline>>);
    return Result<PipelineJob<ComputePipeline>>::Ok(p_Pipeline, p_Layout);
}

void PipelineJob<ComputePipeline>::Dispatch(const VkCommandBuffer p_CommandBuffer, const u32 p_GroupCountX,
                                            const u32 p_GroupCountY, const u32 p_GroupCountZ) const
{
    const LogicalDevice::Proxy &device = m_Pipeline.GetDevice();
    device.Table->CmdDispatch(p_CommandBuffer, p_GroupCountX, p_GroupCountY, p_GroupCountZ);
}

template class IPipelineJob<ComputePipeline>;
template class IPipelineJob<GraphicsPipeline>;

} // namespace VKit::Detail
