#pragma once

#include "vkit/pipeline/pipeline_layout.hpp"
#include "vkit/pipeline/graphics_pipeline.hpp"
#include "vkit/pipeline/compute_pipeline.hpp"

namespace VKit
{
template <typename T>
concept Pipeline = std::is_same_v<T, GraphicsPipeline> || std::is_same_v<T, ComputePipeline>;

template <Pipeline Pip> constexpr VkShaderStageFlags DefaultShaderStage() noexcept
{
    if constexpr (std::is_same_v<Pip, GraphicsPipeline>)
        return VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    else
        return VK_SHADER_STAGE_COMPUTE_BIT;
}

template <Pipeline Pip> class VKIT_API IPipelineJob
{
  public:
    IPipelineJob() noexcept = default;
    IPipelineJob(const Pip &p_Pipeline, const PipelineLayout &p_Layout) noexcept;

    void UpdateDescriptorSet(u32 p_Index, VkDescriptorSet p_DescriptorSet) noexcept;

    template <typename T>
    void UpdatePushConstantRange(u32 p_Index, const T *p_Data,
                                 const VkShaderStageFlags p_Stages = DefaultShaderStage<Pip>()) noexcept
    {
        m_PushData[p_Index] = PushDataInfo{p_Data, sizeof(T), p_Stages};
    }

    void Bind(VkCommandBuffer p_CommandBuffer, u32 p_FirstSet = 0,
              std::span<const u32> p_DynamicOffsets = {}) const noexcept;

    explicit(false) operator bool() const noexcept;

  private:
    struct PushDataInfo
    {
        const void *Data = nullptr;
        u32 Size = 0;
        VkShaderStageFlags Stages = 0;
    };

    Pip m_Pipeline{};
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    TKit::StaticArray8<VkDescriptorSet> m_DescriptorSets;
    TKit::StaticArray4<PushDataInfo> m_PushData;
};

template <Pipeline Pip> class PipelineJob;

template <> class VKIT_API PipelineJob<GraphicsPipeline> final : public IPipelineJob<GraphicsPipeline>
{
  public:
    using IPipelineJob<GraphicsPipeline>::IPipelineJob;

    void Draw(VkCommandBuffer p_CommandBuffer, u32 p_VertexCount, u32 p_InstanceCount = 1, u32 p_FirstVertex = 0,
              u32 p_Firstinstance = 0) const noexcept;
    void DrawIndexed(VkCommandBuffer commandBuffer, u32 p_IndexCount, u32 p_InstanceCount = 1, u32 p_FirstIndex = 0,
                     i32 p_VertexOffset = 0, u32 p_FirstInstance = 0) const noexcept;
};

template <> class VKIT_API PipelineJob<ComputePipeline> final : public IPipelineJob<ComputePipeline>
{
  public:
    using IPipelineJob<ComputePipeline>::IPipelineJob;

    void Dispatch(VkCommandBuffer p_CommandBuffer, u32 p_GroupCountX, u32 p_GroupCountY,
                  u32 p_GroupCountZ) const noexcept;
};

using GraphicsJob = PipelineJob<GraphicsPipeline>;
using ComputeJob = PipelineJob<ComputePipeline>;

} // namespace VKit