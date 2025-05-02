#pragma once

#include "vkit/pipeline/shader.hpp"

namespace VKit
{
/**
 * @brief Represents a Vulkan compute pipeline.
 *
 * Manages the creation, destruction, and usage of a compute pipeline, which is
 * used to execute compute shaders on the GPU.
 */
class VKIT_API ComputePipeline
{
  public:
    struct Specs
    {
        VkPipelineLayout Layout = VK_NULL_HANDLE;
        VkShaderModule ComputeShader = VK_NULL_HANDLE;
        const char *EntryPoint = "main";
        VkPipelineCache Cache = VK_NULL_HANDLE;
    };

    /**
     * @brief Creates a compute pipeline based on the provided specifications.
     *
     * Initializes a compute pipeline with the given logical device and configuration.
     *
     * @param p_Device The logical device proxy for Vulkan operations.
     * @param p_Specs The specifications for the compute pipeline.
     * @return A `Result` containing the created `ComputePipeline` or an error.
     */
    static Result<ComputePipeline> Create(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs) noexcept;

    /**
     * @brief Creates multiple compute pipelines in a batch.
     *
     * Initializes multiple compute pipelines using the provided specifications and logical device.
     *
     * @param p_Device The logical device proxy for Vulkan operations.
     * @param p_Specs A span of specifications for the compute pipelines.
     * @param p_Pipelines A span to store the created pipelines.
     * @return A `Result` indicating success or failure for the batch operation.
     */
    static Result<> Create(const LogicalDevice::Proxy &p_Device, TKit::Span<const Specs> p_Specs,
                           TKit::Span<ComputePipeline> p_Pipelines, VkPipelineCache p_Cache = VK_NULL_HANDLE) noexcept;

    ComputePipeline() noexcept = default;
    ComputePipeline(const LogicalDevice::Proxy &p_Device, VkPipeline p_Pipeline) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    /**
     * @brief Binds the compute pipeline to a command buffer.
     *
     * Prepares the pipeline for execution by binding it to the specified command buffer.
     *
     * @param p_CommandBuffer The Vulkan command buffer to bind the pipeline to.
     */
    void Bind(VkCommandBuffer p_CommandBuffer) const noexcept;

    const LogicalDevice::Proxy &GetDevice() const noexcept;
    VkPipeline GetHandle() const noexcept;
    explicit(false) operator VkPipeline() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};
} // namespace VKit