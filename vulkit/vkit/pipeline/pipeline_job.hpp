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

/**
 * @brief A pipeline job whose purpose is to automate the binding of pipelines and descriptor sets.
 *
 * This class helps with the resource management related to pipelines and descriptor sets, allowing
 * for easier bindings of said resources and the automation of push constant updates.
 *
 * @tparam Pip The type of pipeline to bind. Either `GraphicsPipeline` or `ComputePipeline`.
 */
template <Pipeline Pip> class IPipelineJob
{
  public:
    IPipelineJob() = default;
    IPipelineJob(const Pip &p_Pipeline, const PipelineLayout &p_Layout);

    /**
     * @brief Updates the descriptor set at the specified index.
     *
     * Note that you may be ineterested in calling this method before binding if your render system has more than one
     * frame in flight.
     *
     * @param p_Index The index of the descriptor set to update.
     * @param p_DescriptorSet The descriptor set to update.
     */
    void UpdateDescriptorSet(u32 p_Index, VkDescriptorSet p_DescriptorSet);

    /**
     * @brief Updates the push constant range at the specified index.
     *
     * This method is used to update the push constant range data before binding the pipeline.
     *
     * @tparam T The type of the data to update.
     * @param p_Index The index of the push constant range to update.
     * @param p_Data The data to update the push constant range with.
     * @param p_Stages The shader stages to update the push constant range for.
     */
    template <typename T>
    void UpdatePushConstantRange(u32 p_Index, const T *p_Data,
                                 const VkShaderStageFlags p_Stages = DefaultShaderStage<Pip>())
    {
        m_PushData[p_Index] = PushDataInfo{p_Data, sizeof(T), p_Stages};
    }

    /**
     * @brief Binds the pipeline job to a command buffer.
     *
     * Prepares the pipeline job for rendering by binding the pipeline and descriptor sets to the specified command
     * buffer.
     *
     * @param p_CommandBuffer The Vulkan command buffer to bind the pipeline job to.
     * @param p_FirstSet The first descriptor set to bind.
     * @param p_DynamicOffsets The dynamic offsets to use for the descriptor sets.
     */
    void Bind(VkCommandBuffer p_CommandBuffer, u32 p_FirstSet = 0,
              TKit::Span<const u32> p_DynamicOffsets = {}) const;

    explicit(false) operator bool() const;

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
    TKit::StaticArray8<VkDescriptorSet> m_DescriptorSets;
    TKit::StaticArray4<PushDataInfo> m_PushData;
};

template <Pipeline Pip> class PipelineJob;

/**
 * @brief A pipeline job whose purpose is to automate the binding of pipelines and descriptor sets.
 *
 * This class helps with the resource management related to pipelines and descriptor sets, allowing
 * for easier bindings of said resources and the automation of push constant updates.
 *
 * The graphics variant uses a graphics pipeline and is suited for rendering operations.
 *
 */
template <> class VKIT_API PipelineJob<GraphicsPipeline> final : public IPipelineJob<GraphicsPipeline>
{
  public:
    using IPipelineJob<GraphicsPipeline>::IPipelineJob;

    static Result<PipelineJob> Create(const GraphicsPipeline &p_Pipeline, const PipelineLayout &p_Layout);

    /**
     * @brief A simple wrapper around `vkCmdDraw` to draw the pipeline job.
     *
     * This method is used to draw the pipeline job with the specified parameters.
     *
     * @param p_CommandBuffer The Vulkan command buffer to draw the pipeline job with.
     * @param p_VertexCount The number of vertices to draw.
     * @param p_InstanceCount The number of instances to draw.
     * @param p_FirstVertex The first vertex to draw.
     * @param p_FirstInstance The first instance to draw.
     */
    void Draw(VkCommandBuffer p_CommandBuffer, u32 p_VertexCount, u32 p_InstanceCount = 1, u32 p_FirstVertex = 0,
              u32 p_Firstinstance = 0) const;

    /**
     * @brief A simple wrapper around `vkCmdDrawIndexed` to draw the pipeline job with indices.
     *
     * This method is used to draw the pipeline job with the specified parameters.
     *
     * @param p_CommandBuffer The Vulkan command buffer to draw the pipeline job with.
     * @param p_IndexCount The number of indices to draw.
     * @param p_InstanceCount The number of instances to draw.
     * @param p_FirstIndex The first index to draw.
     * @param p_VertexOffset The vertex offset to draw.
     * @param p_FirstInstance The first instance to draw.
     */
    void DrawIndexed(VkCommandBuffer commandBuffer, u32 p_IndexCount, u32 p_InstanceCount = 1, u32 p_FirstIndex = 0,
                     i32 p_VertexOffset = 0, u32 p_FirstInstance = 0) const;
};

/**
 * @brief A pipeline job whose purpose is to automate the binding of pipelines and descriptor sets.
 *
 * This class helps with the resource management related to pipelines and descriptor sets, allowing
 * for easier bindings of said resources and the automation of push constant updates.
 *
 * The compute variant uses a compute pipeline and is suited for compute operations.
 *
 */
template <> class VKIT_API PipelineJob<ComputePipeline> final : public IPipelineJob<ComputePipeline>
{
  public:
    using IPipelineJob<ComputePipeline>::IPipelineJob;

    static Result<PipelineJob> Create(const ComputePipeline &p_Pipeline, const PipelineLayout &p_Layout);

    /**
     * @brief A simple wrapper around `vkCmdDispatch` to dispatch the pipeline job.
     *
     * This method is used to dispatch the pipeline job with the specified parameters.
     *
     * @param p_CommandBuffer The Vulkan command buffer to dispatch the pipeline job with.
     * @param p_GroupCountX The number of groups in the X dimension.
     * @param p_GroupCountY The number of groups in the Y dimension.
     * @param p_GroupCountZ The number of groups in the Z dimension.
     */
    void Dispatch(VkCommandBuffer p_CommandBuffer, u32 p_GroupCountX, u32 p_GroupCountY,
                  u32 p_GroupCountZ) const;
};
} // namespace VKit::Detail

namespace VKit
{
using GraphicsJob = Detail::PipelineJob<GraphicsPipeline>;
using ComputeJob = Detail::PipelineJob<ComputePipeline>;

} // namespace VKit
