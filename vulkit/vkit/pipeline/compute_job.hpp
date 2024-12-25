#pragma once

#include "vkit/pipeline/pipeline_layout.hpp"
#include "vkit/pipeline/compute_pipeline.hpp"

namespace VKit
{
class ComputeJob
{
  public:
    ComputeJob() noexcept = default;
    ComputeJob(const ComputePipeline &p_Pipeline, VkPipelineLayout p_Layout) noexcept;

    void AddDescriptorSet(VkDescriptorSet p_DescriptorSet) noexcept;
    void UpdateDescriptorSet(u32 p_Index, VkDescriptorSet p_DescriptorSet) noexcept;

    template <typename T> void AddPushConstantRange(const T *p_Data) noexcept
    {
        m_PushData.push_back(PushDataInfo{p_Data, sizeof(T)});
    }
    template <typename T> void UpdatePushConstantRange(u32 p_Index, const T *p_Data) noexcept
    {
        m_PushData[p_Index] = PushDataInfo{p_Data, sizeof(T)};
    }

    void Bind(VkCommandBuffer p_CommandBuffer, std::span<const u32> p_DynamicOffsets = {}) const noexcept;
    void Dispatch(VkCommandBuffer p_CommandBuffer, u32 p_GroupCountX, u32 p_GroupCountY,
                  u32 p_GroupCountZ) const noexcept;

    explicit(false) operator bool() const noexcept;

  private:
    struct PushDataInfo
    {
        const void *Data = nullptr;
        u32 Size = 0;
    };

    ComputePipeline m_Pipeline{};
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    TKit::StaticArray8<VkDescriptorSet> m_DescriptorSets;
    TKit::StaticArray4<PushDataInfo> m_PushData;
};
} // namespace VKit