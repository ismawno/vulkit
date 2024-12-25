#include "vkit/core/pch.hpp"
#include "vkit/pipeline/pipeline_job.hpp"
#include "vkit/descriptors/descriptor_set.hpp"

namespace VKit
{
template <Pipeline Pip>
IPipelineJob<Pip>::IPipelineJob(const Pip &p_Pipeline, const PipelineLayout &p_Layout) noexcept
    : m_Pipeline(p_Pipeline), m_Layout(p_Layout)
{
    m_DescriptorSets.resize(p_Layout.GetInfo().DescriptorSetLayouts.size(), VK_NULL_HANDLE);
    m_PushData.resize(p_Layout.GetInfo().PushConstantRanges.size());
}

template <Pipeline Pip>
void IPipelineJob<Pip>::UpdateDescriptorSet(u32 p_Index, VkDescriptorSet p_DescriptorSet) noexcept
{
    m_DescriptorSets[p_Index] = p_DescriptorSet;
}

template <Pipeline Pip>
void IPipelineJob<Pip>::Bind(const VkCommandBuffer p_CommandBuffer, u32 p_FirstSet,
                             const std::span<const u32> p_DynamicOffsets) const noexcept
{
    m_Pipeline.Bind(p_CommandBuffer);
    u32 offset = 0;
    const u32 pushCount = static_cast<u32>(m_PushData.size());

    // Data may not need to be pushed every frame... but I guess it is a small price to pay for the flexibility
    for (u32 i = 0; i < pushCount; ++i)
    {
        const PushDataInfo &info = m_PushData[i];
        if (!info.Data)
            continue;
        vkCmdPushConstants(p_CommandBuffer, m_Layout, info.Stages, offset, info.Size, info.Data);
        offset += info.Size;
    }

    TKit::StaticArray8<VkDescriptorSet> descriptorSets;
    for (const VkDescriptorSet set : m_DescriptorSets)
        if (set)
            descriptorSets.push_back(set);
    if (descriptorSets.empty())
        return;

    if constexpr (std::is_same_v<Pip, GraphicsPipeline>)
        DescriptorSet::Bind(p_CommandBuffer, m_DescriptorSets, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Layout, p_FirstSet,
                            p_DynamicOffsets);
    else
        DescriptorSet::Bind(p_CommandBuffer, m_DescriptorSets, VK_PIPELINE_BIND_POINT_COMPUTE, m_Layout, p_FirstSet,
                            p_DynamicOffsets);
}
template <Pipeline Pip> IPipelineJob<Pip>::operator bool() const noexcept
{
    return m_Pipeline;
}

void PipelineJob<GraphicsPipeline>::Draw(const VkCommandBuffer p_CommandBuffer, const u32 p_VertexCount,
                                         const u32 p_InstanceCount, const u32 p_FirstVertex,
                                         const u32 p_FirstInstance) const noexcept
{
    vkCmdDraw(p_CommandBuffer, p_VertexCount, p_InstanceCount, p_FirstVertex, p_FirstInstance);
}
void PipelineJob<GraphicsPipeline>::DrawIndexed(const VkCommandBuffer p_CommandBuffer, const u32 p_IndexCount,
                                                const u32 p_InstanceCount, const u32 p_FirstIndex,
                                                const i32 p_VertexOffset, const u32 p_FirstInstance) const noexcept
{
    vkCmdDrawIndexed(p_CommandBuffer, p_IndexCount, p_InstanceCount, p_FirstIndex, p_VertexOffset, p_FirstInstance);
}

void PipelineJob<ComputePipeline>::Dispatch(const VkCommandBuffer p_CommandBuffer, const u32 p_GroupCountX,
                                            const u32 p_GroupCountY, const u32 p_GroupCountZ) const noexcept
{
    vkCmdDispatch(p_CommandBuffer, p_GroupCountX, p_GroupCountY, p_GroupCountZ);
}

template class IPipelineJob<ComputePipeline>;
template class IPipelineJob<GraphicsPipeline>;

} // namespace VKit