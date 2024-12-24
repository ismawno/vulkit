#include "vkit/core/pch.hpp"
#include "vkit/pipeline/compute_job.hpp"
#include "vkit/descriptors/descriptor_set.hpp"

namespace VKit
{
ComputeJob::ComputeJob(const ComputePipeline &p_Pipeline, const VkPipelineLayout p_Layout) noexcept
    : m_Pipeline(p_Pipeline), m_Layout(p_Layout)
{
}

void ComputeJob::AddDescriptorSet(VkDescriptorSet p_DescriptorSet) noexcept
{
    m_DescriptorSets.push_back(p_DescriptorSet);
}
void ComputeJob::UpdateDescriptorSet(u32 p_Index, VkDescriptorSet p_DescriptorSet) noexcept
{
    m_DescriptorSets[p_Index] = p_DescriptorSet;
}

void ComputeJob::Bind(const VkCommandBuffer p_CommandBuffer, const std::span<const u32> p_DynamicOffsets) const noexcept
{
    m_Pipeline.Bind(p_CommandBuffer);
    TKit::StaticArray8<VkDescriptorSet> descriptorSets;

    const u32 descriptorCount = static_cast<u32>(m_DescriptorSets.size());
    for (u32 i = 0; i < descriptorCount; ++i)
        if (m_DescriptorSets[i])
            descriptorSets.push_back(m_DescriptorSets[i]);

    DescriptorSet::Bind(p_CommandBuffer, descriptorSets, VK_PIPELINE_BIND_POINT_COMPUTE, m_Layout, 0, p_DynamicOffsets);

    u32 offset = 0;
    const u32 pushCount = static_cast<u32>(m_PushData.size());
    for (u32 i = 0; i < pushCount; ++i)
    {
        const PushDataInfo &info = m_PushData[i];
        if (!info.Data)
            continue;
        vkCmdPushConstants(p_CommandBuffer, m_Layout, VK_SHADER_STAGE_COMPUTE_BIT, offset, info.Size, info.Data);
        offset += info.Size;
    }
}
void ComputeJob::Dispatch(const VkCommandBuffer p_CommandBuffer, const u32 p_GroupCountX, const u32 p_GroupCountY,
                          const u32 p_GroupCountZ) const noexcept
{
    vkCmdDispatch(p_CommandBuffer, p_GroupCountX, p_GroupCountY, p_GroupCountZ);
}
ComputeJob::operator bool() const noexcept
{
    return m_Pipeline;
}

} // namespace VKit