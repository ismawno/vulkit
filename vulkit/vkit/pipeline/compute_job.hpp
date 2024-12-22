#pragma once

#include "vkit/pipeline/pipeline_layout.hpp"
#include "vkit/pipeline/compute_pipeline.hpp"

namespace VKit
{
class ComputeJob
{
  public:
    static Result<ComputeJob> Create(const LogicalDevice::Proxy &p_Device, const PipelineLayout &p_Layout,
                                     const Shader &p_ComputeShader) noexcept;

    ComputeJob() noexcept = default;
    ComputeJob(const PipelineLayout &p_Layout, const ComputePipeline &p_Pipeline) noexcept;

    void UpdateDescriptorSet(u32 p_Index, VkDescriptorSet p_DescriptorSet) noexcept;
    void UpdateDescriptorSet(VkDescriptorSet p_DescriptorSet) noexcept;

    template <typename T> void UpdatePushConstantRange(u32 p_Index, const T *p_Data) noexcept
    {
        m_PushData[p_Index] = {p_Data, sizeof(T)};
    }
    template <typename T> void UpdatePushConstantRange(const T *p_Data) noexcept
    {
        UpdatePushConstantRange(0, p_Data);
    }

    void Bind(VkCommandBuffer p_CommandBuffer, std::span<const u32> p_DynamicOffsets = {}) const noexcept;
    void Dispatch(VkCommandBuffer p_CommandBuffer, u32 p_GroupCountX, u32 p_GroupCountY,
                  u32 p_GroupCountZ) const noexcept;

  private:
    struct PushDataInfo
    {
        const void *Data = nullptr;
        u32 Size = 0;
    };

    PipelineLayout m_Layout{};
    ComputePipeline m_Pipeline{};
    TKit::StaticArray8<VkDescriptorSet> m_DescriptorSets;
    TKit::StaticArray4<PushDataInfo> m_PushData;
};
} // namespace VKit