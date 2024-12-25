#include "vkit/core/pch.hpp"
#include "vkit/pipeline/graphics_job.hpp"
#include "vkit/descriptors/descriptor_set.hpp"

namespace VKit
{
GraphicsJob::GraphicsJob(const GraphicsPipeline &p_Pipeline, VkPipelineLayout p_Layout, u32 p_FramesInFlight) noexcept
    : m_Pipeline(p_Pipeline), m_Layout(p_Layout)
{
    m_DescriptorSets.resize(p_FramesInFlight);
}

void GraphicsJob::AddDescriptorSet(const std::span<const VkDescriptorSet> p_Sets) noexcept
{
    TKIT_ASSERT(p_Sets.size() == m_DescriptorSets.size(), "Descriptor set count does not match the frame count");
    for (usize i = 0; i < m_DescriptorSets.size(); ++i)
        m_DescriptorSets[i].push_back(p_Sets[i]);
}
void GraphicsJob::UpdateDescriptorSet(const u32 p_Index, const std::span<const VkDescriptorSet> p_Sets) noexcept
{
    TKIT_ASSERT(p_Sets.size() == m_DescriptorSets.size(), "Descriptor set count does not match the frame count");
    for (usize i = 0; i < m_DescriptorSets.size(); ++i)
        m_DescriptorSets[i][p_Index] = p_Sets[i];
}

void GraphicsJob::Bind(const VkCommandBuffer p_CommandBuffer, const u32 p_FrameIndex,
                       const std::span<const u32> p_DynamicOffsets) const noexcept
{
    m_Pipeline.Bind(p_CommandBuffer);
    const PerFrameDescriptor &descriptorSets = m_DescriptorSets[p_FrameIndex];

    DescriptorSet::Bind(p_CommandBuffer, descriptorSets, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Layout, 0,
                        p_DynamicOffsets);

    u32 offset = 0;
    const u32 pushCount = static_cast<u32>(m_PushData.size());

    // Data may not need to be pushed every frame... but I guess it is a small price to pay for the flexibility
    for (u32 i = 0; i < pushCount; ++i)
    {
        const PushDataInfo &info = m_PushData[i];
        vkCmdPushConstants(p_CommandBuffer, m_Layout, info.Stages, offset, info.Size, info.Data);
        offset += info.Size;
    }
}

void GraphicsJob::Draw(const VkCommandBuffer p_CommandBuffer, const u32 p_VertexCount, const u32 p_InstanceCount,
                       const u32 p_FirstVertex, const u32 p_FirstInstance) noexcept
{
    vkCmdDraw(p_CommandBuffer, p_VertexCount, p_InstanceCount, p_FirstVertex, p_FirstInstance);
}
void GraphicsJob::DrawIndexed(const VkCommandBuffer p_CommandBuffer, const u32 p_IndexCount, const u32 p_InstanceCount,
                              const u32 p_FirstIndex, const i32 p_VertexOffset, const u32 p_FirstInstance) noexcept
{
    vkCmdDrawIndexed(p_CommandBuffer, p_IndexCount, p_InstanceCount, p_FirstIndex, p_VertexOffset, p_FirstInstance);
}
GraphicsJob::operator bool() const noexcept
{
    return m_Pipeline;
}

} // namespace VKit