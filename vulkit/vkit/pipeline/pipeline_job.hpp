#pragma once

#ifndef VKIT_ENABLE_PIPELINE_JOB
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_PIPELINE_JOB"
#endif

#include "vkit/pipeline/pipeline_layout.hpp"
#include "vkit/pipeline/graphics_pipeline.hpp"
#include "vkit/pipeline/compute_pipeline.hpp"

namespace VKit::Detail
{
template <typename T>
concept Pipeline = std::is_same_v<T, GraphicsPipeline> || std::is_same_v<T, ComputePipeline>;
template <Pipeline Pip> constexpr VkShaderStageFlags DefaultShaderStage()
{
    if constexpr (std::is_same_v<Pip, GraphicsPipeline>)
        return VK_SHADER_STAGE_ALL_GRAPHICS;
    else
        return VK_SHADER_STAGE_COMPUTE_BIT;
}

template <Pipeline Pip> class IPipelineJob
{
  public:
    IPipelineJob() = default;
    IPipelineJob(const Pip &p_Pipeline, const PipelineLayout &p_Layout);

    void UpdateDescriptorSet(u32 p_Index, VkDescriptorSet p_DescriptorSet)
    {
        m_DescriptorSets[p_Index] = p_DescriptorSet;
    }

    template <typename T>
    void UpdatePushConstantRange(u32 p_Index, const T *p_Data,
                                 const VkShaderStageFlags p_Stages = DefaultShaderStage<Pip>())
    {
        m_PushData[p_Index] = PushDataInfo{p_Data, sizeof(T), p_Stages};
    }

    void Bind(VkCommandBuffer p_CommandBuffer, u32 p_FirstSet = 0, TKit::Span<const u32> p_DynamicOffsets = {}) const;

    operator bool() const;

  protected:
    Pip m_Pipeline{};

  private:
    struct PushDataInfo
    {
        const void *Data = nullptr;
        u32 Size = 0;
        VkShaderStageFlags Stages = 0;
    };

    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    TKit::Array8<VkDescriptorSet> m_DescriptorSets;
    TKit::Array4<PushDataInfo> m_PushData;
};

template <Pipeline Pip> class PipelineJob;

template <> class VKIT_API PipelineJob<GraphicsPipeline> final : public IPipelineJob<GraphicsPipeline>
{
  public:
    using IPipelineJob<GraphicsPipeline>::IPipelineJob;

    static Result<PipelineJob> Create(const GraphicsPipeline &p_Pipeline, const PipelineLayout &p_Layout);

    void Draw(VkCommandBuffer p_CommandBuffer, u32 p_VertexCount, u32 p_InstanceCount = 1, u32 p_FirstVertex = 0,
              u32 p_Firstinstance = 0) const;

    void DrawIndexed(VkCommandBuffer commandBuffer, u32 p_IndexCount, u32 p_InstanceCount = 1, u32 p_FirstIndex = 0,
                     i32 p_VertexOffset = 0, u32 p_FirstInstance = 0) const;
};

template <> class VKIT_API PipelineJob<ComputePipeline> final : public IPipelineJob<ComputePipeline>
{
  public:
    using IPipelineJob<ComputePipeline>::IPipelineJob;

    static Result<PipelineJob> Create(const ComputePipeline &p_Pipeline, const PipelineLayout &p_Layout);

    void Dispatch(VkCommandBuffer p_CommandBuffer, u32 p_GroupCountX, u32 p_GroupCountY, u32 p_GroupCountZ) const;
};
} // namespace VKit::Detail

namespace VKit
{
using GraphicsJob = Detail::PipelineJob<GraphicsPipeline>;
using ComputeJob = Detail::PipelineJob<ComputePipeline>;

} // namespace VKit
