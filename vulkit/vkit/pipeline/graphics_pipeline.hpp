#pragma once

#include "vkit/core/alias.hpp"
#include "vkit/backend/logical_device.hpp"
#include "vkit/pipeline/shader.hpp"
#include "tkit/core/non_copyable.hpp"

#include <vulkan/vulkan.hpp>

namespace VKit
{
/**
 * @brief The GraphicsPipeline class encapsulates Vulkan pipeline creation and management.
 *
 * Responsible for creating graphics pipelines based on provided specifications,
 * and provides methods to bind the pipeline for rendering.
 */
class VKIT_API GraphicsPipeline
{
  public:
    /**
     * @brief Struct containing specifications for creating a Vulkan pipeline.
     */
    struct Specs
    {
        /**
         * @brief Constructs a Specs object with default initialization.
         */
        Specs() noexcept;

        /**
         * @brief Populates the pipeline specifications internally.
         *
         * Involves some of the members of the struct to
         * point to other members. Every time the specs are copied, this method should be called.
         *
         */
        void Populate() noexcept;

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

        // Do not need to remain alive on the user end
        Shader VertexShader{};
        Shader FragmentShader{};

        std::span<const VkDynamicState> DynamicStates;
        std::span<const VkVertexInputBindingDescription> BindingDescriptions;
        std::span<const VkVertexInputAttributeDescription> AttributeDescriptions;

        // Automatically populated, user does not need to touch this! (It is easier to leave it here bc of impl details)
        std::array<VkPipelineShaderStageCreateInfo, 2> ShaderStages{};
        VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
    };

    static Result<GraphicsPipeline> Create(const LogicalDevice::Proxy &p_Device, Specs &p_Specs) noexcept;
    static VulkanResult Create(const LogicalDevice::Proxy &p_Device, std::span<Specs> p_Specs,
                               std::span<GraphicsPipeline> p_Pipelines) noexcept;

    GraphicsPipeline() noexcept = default;
    GraphicsPipeline(const LogicalDevice::Proxy &p_Device, VkPipeline p_Pipeline, VkPipelineLayout p_PipelineLayout,
                     const Shader &p_VertexShader, const Shader &p_FragmentShader) noexcept;

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
    LogicalDevice::Proxy m_Device{};
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    Shader m_VertexShader{};
    Shader m_FragmentShader{};
};
} // namespace VKit
