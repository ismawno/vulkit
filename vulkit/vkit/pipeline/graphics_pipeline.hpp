#pragma once

#include "vkit/core/alias.hpp"
#include "vkit/backend/logical_device.hpp"
#include "vkit/pipeline/shader.hpp"
#include "tkit/core/non_copyable.hpp"

#include <vulkan/vulkan.h>

namespace VKit
{
/**
 * @brief Represents a Vulkan graphics pipeline.
 *
 * Handles the creation, management, and binding of graphics pipelines, which are used
 * for rendering in Vulkan. Includes functionality for custom specifications and batch creation.
 */
class VKIT_API GraphicsPipeline
{
  public:
    /**
     * @brief Configuration for creating a Vulkan graphics pipeline.
     *
     * Contains all the necessary settings for pipeline creation, including shaders,
     * layout, render pass, and state settings. Provides utility methods for internal
     * setup and pipeline creation.
     */

    struct Specs
    {
        /**
         * @brief Constructs a Specs object with default initialization.
         */
        Specs() noexcept;

        /**
         * @brief Populates the internal state of the pipeline specifications.
         *
         * Ensures that pointers within the specs point to valid internal members. This
         * method should be called after copying or modifying the specs to maintain consistency.
         */
        void Populate() noexcept;

        /**
         * @brief Generates the `VkGraphicsPipelineCreateInfo` object.
         *
         * Constructs the Vulkan graphics pipeline creation info based on the specs.
         * This is used internally during pipeline creation.
         *
         * @return A result containing the created `VkGraphicsPipelineCreateInfo` or an error.
         */
        Result<VkGraphicsPipelineCreateInfo> CreatePipelineInfo() noexcept;

        VkPipelineViewportStateCreateInfo ViewportInfo{};
        VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo{};
        VkPipelineRasterizationStateCreateInfo RasterizationInfo{};
        VkPipelineMultisampleStateCreateInfo MultisampleInfo{};
        VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo ColorBlendInfo{};
        VkPipelineDepthStencilStateCreateInfo DepthStencilInfo{};

        VkPipelineLayout Layout = VK_NULL_HANDLE;
        VkPipelineCache Cache = VK_NULL_HANDLE;
        VkPipelineDynamicStateCreateInfo DynamicStateInfo{};
        VkRenderPass RenderPass = VK_NULL_HANDLE;

        u32 Subpass = 0;

        Shader VertexShader{};
        Shader FragmentShader{};

        std::span<const VkDynamicState> DynamicStates;
        std::span<const VkVertexInputBindingDescription> BindingDescriptions;
        std::span<const VkVertexInputAttributeDescription> AttributeDescriptions;

        // Automatically populated, user does not need to touch this! (It is easier to leave it here bc of impl details)
        std::array<VkPipelineShaderStageCreateInfo, 2> ShaderStages{};
        VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
    };

    /**
     * @brief Creates a graphics pipeline based on the provided specifications.
     *
     * Initializes a Vulkan graphics pipeline using the specified logical device
     * and pipeline configuration.
     *
     * @param p_Device The logical device proxy for Vulkan operations.
     * @param p_Specs The specifications for the graphics pipeline.
     * @return A result containing the created GraphicsPipeline or an error.
     */
    static Result<GraphicsPipeline> Create(const LogicalDevice::Proxy &p_Device, Specs &p_Specs) noexcept;

    /**
     * @brief Creates multiple graphics pipelines in a batch.
     *
     * Initializes multiple Vulkan graphics pipelines using the provided specifications
     * and logical device.
     *
     * @param p_Device The logical device proxy for Vulkan operations.
     * @param p_Specs A span of specifications for the graphics pipelines.
     * @param p_Pipelines A span to store the created pipelines.
     * @return A VulkanResult indicating success or failure for the batch operation.
     */
    static VulkanResult Create(const LogicalDevice::Proxy &p_Device, std::span<Specs> p_Specs,
                               std::span<GraphicsPipeline> p_Pipelines) noexcept;

    GraphicsPipeline() noexcept = default;
    GraphicsPipeline(const LogicalDevice::Proxy &p_Device, VkPipeline p_Pipeline, VkPipelineLayout p_PipelineLayout,
                     const Shader &p_VertexShader, const Shader &p_FragmentShader) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    /**
     * @brief Binds the graphics pipeline to a command buffer.
     *
     * Prepares the pipeline for rendering by binding it to the specified command buffer.
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
    LogicalDevice::Proxy m_Device{};
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    Shader m_VertexShader{};
    Shader m_FragmentShader{};
};
} // namespace VKit
