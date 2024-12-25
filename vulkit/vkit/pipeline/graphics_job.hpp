#pragma once

#include "vkit/pipeline/pipeline_layout.hpp"
#include "vkit/pipeline/graphics_pipeline.hpp"

namespace VKit
{
class GraphicsJob
{
  public:
    GraphicsJob() noexcept = default;
    GraphicsJob(const GraphicsPipeline &p_Pipeline, VkPipelineLayout p_Layout, u32 p_FramesInFlight) noexcept;

    void AddDescriptorSet(std::span<const VkDescriptorSet> p_Sets) noexcept;
    void UpdateDescriptorSet(u32 p_Index, std::span<const VkDescriptorSet> p_Sets) noexcept;

    template <typename T> void AddPushConstantRange(const T *p_Data, const VkShaderStageFlags p_Stages) noexcept
    {
        m_PushData.push_back(PushDataInfo{p_Data, sizeof(T), p_Stages});
    }
    template <typename T>
    void UpdatePushConstantRange(u32 p_Index, const T *p_Data, const VkShaderStageFlags p_Stages) noexcept
    {
        m_PushData[p_Index] = PushDataInfo{p_Data, sizeof(T), p_Stages};
    }

    void Bind(VkCommandBuffer p_CommandBuffer, u32 p_FrameIndex,
              std::span<const u32> p_DynamicOffsets = {}) const noexcept;

    void Draw(VkCommandBuffer p_CommandBuffer, u32 p_VertexCount, u32 p_InstanceCount = 1, u32 p_FirstVertex = 0,
              u32 p_Firstinstance = 0) noexcept;
    void DrawIndexed(VkCommandBuffer commandBuffer, u32 p_IndexCount, u32 p_InstanceCount = 1, u32 p_FirstIndex = 0,
                     i32 p_VertexOffset = 0, u32 p_FirstInstance = 0) noexcept;

    explicit(false) operator bool() const noexcept;

  private:
    struct PushDataInfo
    {
        const void *Data = nullptr;
        u32 Size = 0;
        VkShaderStageFlags Stages = 0;
    };
    using PerFrameDescriptor = TKit::StaticArray16<VkDescriptorSet>;

    GraphicsPipeline m_Pipeline{};
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    TKit::StaticArray4<PerFrameDescriptor> m_DescriptorSets;
    TKit::StaticArray4<PushDataInfo> m_PushData;
};
} // namespace VKit