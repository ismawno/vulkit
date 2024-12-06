#pragma once

#include "vkit/pipeline/shader.hpp"

namespace VKit
{
class ComputePipeline
{
  public:
    struct Specs
    {
        VkPipelineLayout Layout = VK_NULL_HANDLE;
        const Shader *ComputeShader = nullptr;
        const char *EntryPoint = "main";
        VkPipelineCache Cache = VK_NULL_HANDLE;
    };

    static Result<ComputePipeline> Create(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs) noexcept;
    static VulkanResult Create(const LogicalDevice::Proxy &p_Device, std::span<const Specs> p_Specs,
                               std::span<ComputePipeline> p_Pipelines) noexcept;

    ComputePipeline(const LogicalDevice::Proxy &p_Device, VkPipeline p_Pipeline, VkPipelineLayout p_PipelineLayout,
                    const Shader &p_ComputeShader) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    /**
     * @brief Binds the pipeline to the specified command buffer.
     *
     * @param p_CommandBuffer The Vulkan command buffer to bind the pipeline to.
     */
    void Bind(VkCommandBuffer p_CommandBuffer) const noexcept;

    /**
     * @brief Retrieves the pipeline layout.
     *
     * @return The Vulkan pipeline layout.
     */
    VkPipelineLayout GetLayout() const noexcept;

    VkPipeline GetPipeline() const noexcept;
    explicit(false) operator VkPipeline() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    LogicalDevice::Proxy m_Device;
    VkPipeline m_Pipeline;
    VkPipelineLayout m_Layout;
    Shader m_ComputeShader;
};
} // namespace VKit