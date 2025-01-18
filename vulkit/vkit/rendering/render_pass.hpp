#pragma once

#include "vkit/backend/logical_device.hpp"
#include "vkit/core/vma.hpp"
#include "tkit/container/static_array.hpp"

namespace VKit
{
struct Attachment
{
    enum FlagBits : u8
    {
        Flag_Color = 1 << 0,
        Flag_Depth = 1 << 1,
        Flag_Stencil = 1 << 2,
        Flag_Input = 1 << 3,
        Flag_Sampled = 1 << 4,
    };
    using Flags = u8;

    VkAttachmentDescription Description{};
    Flags TypeFlags;
};

/**
 * @brief Represents a Vulkan render pass and its associated resources.
 *
 * Manages the configuration, creation, and destruction of Vulkan render passes, along with helper utilities for
 * attachments, subpasses, and dependencies. Also supports resource creation for associated frame buffers and images.
 */
class RenderPass
{
  public:
    class Builder;

  private:
    class AttachmentBuilder
    {
      public:
        AttachmentBuilder(Builder *p_Builder, Attachment::Flags p_TypeFlags) noexcept;

        AttachmentBuilder &SetLoadOperation(
            VkAttachmentLoadOp p_Operation,
            VkAttachmentLoadOp p_StencilOperation = VK_ATTACHMENT_LOAD_OP_MAX_ENUM) noexcept;
        AttachmentBuilder &SetStoreOperation(
            VkAttachmentStoreOp p_Operation,
            VkAttachmentStoreOp p_StencilOperation = VK_ATTACHMENT_STORE_OP_MAX_ENUM) noexcept;

        AttachmentBuilder &SetStencilLoadOperation(VkAttachmentLoadOp p_Operation) noexcept;
        AttachmentBuilder &SetStencilStoreOperation(VkAttachmentStoreOp p_Operation) noexcept;

        AttachmentBuilder &RequestFormat(VkFormat p_Format) noexcept;
        AttachmentBuilder &AllowFormat(VkFormat p_Format) noexcept;

        AttachmentBuilder &SetLayouts(VkImageLayout p_InitialLayout, VkImageLayout p_FinalLayout) noexcept;
        AttachmentBuilder &SetInitialLayout(VkImageLayout p_Layout) noexcept;
        AttachmentBuilder &SetFinalLayout(VkImageLayout p_Layout) noexcept;

        AttachmentBuilder &SetSampleCount(VkSampleCountFlagBits p_SampleCount) noexcept;
        AttachmentBuilder &SetFlags(VkAttachmentDescriptionFlags p_Flags) noexcept;

        Builder &EndAttachment() noexcept;

      private:
        Builder *m_Builder;
        Attachment m_Attachment{};
        TKit::StaticArray16<VkFormat> m_Formats;

        friend class Builder;
    };

    class SubpassBuilder
    {
      public:
        SubpassBuilder(Builder *p_Builder, VkPipelineBindPoint p_BindPoint) noexcept;

        SubpassBuilder &AddColorAttachment(u32 p_AttachmentIndex,
                                           VkImageLayout p_Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                           u32 p_ResolveIndex = UINT32_MAX) noexcept;
        SubpassBuilder &AddColorAttachment(u32 p_AttachmentIndex, u32 p_ResolveIndex) noexcept;

        SubpassBuilder &AddInputAttachment(u32 p_AttachmentIndex, VkImageLayout p_Layout) noexcept;
        SubpassBuilder &AddPreserveAttachment(u32 p_AttachmentIndex) noexcept;

        SubpassBuilder &SetDepthStencilAttachment(
            u32 p_AttachmentIndex, VkImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) noexcept;

        SubpassBuilder &SetFlags(VkSubpassDescriptionFlags p_Flags) noexcept;

        Builder &EndSubpass() noexcept;

      private:
        Builder *m_Builder;
        VkSubpassDescription m_Description{};
        TKit::StaticArray8<VkAttachmentReference> m_ColorAttachments{};
        TKit::StaticArray8<VkAttachmentReference> m_InputAttachments{};
        TKit::StaticArray8<u32> m_PreserveAttachments{};
        TKit::StaticArray8<VkAttachmentReference> m_ResolveAttachments{};
        VkAttachmentReference m_DepthStencilAttachment{};

        friend class Builder;
    };

    class DependencyBuilder
    {
      public:
        DependencyBuilder(Builder *p_Builder, u32 p_SourceSubpass, u32 p_DestinationSubpass) noexcept;

        DependencyBuilder &SetStageMask(VkPipelineStageFlags p_SourceStage,
                                        VkPipelineStageFlags p_DestinationStage) noexcept;
        DependencyBuilder &SetAccessMask(VkAccessFlags p_SourceAccess, VkAccessFlags p_DestinationAccess) noexcept;

        DependencyBuilder &SetFlags(VkDependencyFlags p_Flags) noexcept;

        Builder &EndDependency() noexcept;

      private:
        Builder *m_Builder;
        VkSubpassDependency m_Dependency{};

        friend class Builder;
    };

  public:
    /**
     * @brief A utility for constructing Vulkan render passes.
     *
     * Provides methods for configuring attachments, subpasses, and dependencies, while allowing fine-grained control
     * over render pass creation flags and resource management.
     */
    class Builder
    {
      public:
        explicit Builder(const LogicalDevice *p_Device, u32 p_ImageCount) noexcept;

        Result<RenderPass> Build() const noexcept;

        AttachmentBuilder &BeginAttachment(Attachment::Flags p_Flags) noexcept;

        SubpassBuilder &BeginSubpass(VkPipelineBindPoint p_BindPoint) noexcept;
        DependencyBuilder &BeginDependency(u32 p_SourceSubpass, u32 p_DestinationSubpass) noexcept;

        Builder &SetFlags(VkRenderPassCreateFlags p_Flags) noexcept;
        Builder &AddFlags(VkRenderPassCreateFlags p_Flags) noexcept;
        Builder &RemoveFlags(VkRenderPassCreateFlags p_Flags) noexcept;

        Builder &SetAllocator(VmaAllocator p_Allocator) noexcept;

      private:
        const LogicalDevice *m_Device;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VkRenderPassCreateFlags m_Flags = 0;
        u32 m_ImageCount;

        TKit::StaticArray16<AttachmentBuilder> m_Attachments{};
        TKit::StaticArray8<SubpassBuilder> m_Subpasses{};
        TKit::StaticArray8<DependencyBuilder> m_Dependencies{};
    };

    struct ImageData
    {
        VkImage Image;
        VkImageView ImageView;
        VmaAllocation Allocation;
    };

    /**
     * @brief Manages frame buffers and image views associated with a render pass.
     *
     * Can be created with the `CreateResources()` method, which generates frame buffers and image views for each
     * attachment. The user is responsible for destroying the resources when they are no longer needed.
     *
     */
    class Resources
    {
      public:
        void Destroy() noexcept;
        void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

        VkImageView GetImageView(u32 p_ImageIndex, u32 p_AttachmentIndex) const noexcept;
        VkFramebuffer GetFrameBuffer(u32 p_ImageIndex) const noexcept;

      private:
        void destroy() const noexcept;

        LogicalDevice::Proxy m_Device{};
        VmaAllocator m_Allocator = VK_NULL_HANDLE;

        TKit::StaticArray64<ImageData> m_Images;          // size: m_ImageCount * m_Attachments.size()
        TKit::StaticArray4<VkFramebuffer> m_FrameBuffers; // size: m_ImageCount

        friend class RenderPass;
    };

    struct Info
    {
        VmaAllocator Allocator;
        TKit::StaticArray16<Attachment> Attachments;
        u32 ImageCount;
    };

    RenderPass() noexcept = default;
    RenderPass(const LogicalDevice::Proxy &p_Device, VkRenderPass p_RenderPass, const Info &p_Info) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    /**
     * @brief Creates resources for the render pass, including frame buffers and image data.
     *
     * Populates frame buffers and associated images based on the provided extent and a user-defined image creation
     * callback. The `RenderPass` class provides many high-level options for `ImageData` struct creation, including the
     * case where the underlying resource is directly provided by a `SwapChain` image. See the `CreateImageData()`
     * methods for more.
     *
     * @tparam F The type of the callback function used for creating image data.
     * @param p_Extent The dimensions of the frame buffer.
     * @param p_CreateImageData A callback function that generates image data for each attachment. Takes the image index
     * and attachment index as arguments.
     * @param p_FrameBufferLayers The number of layers for each frame buffer (default: 1).
     * @return A `Result` containing the created `Resources` or an error.
     */
    template <typename F>
    Result<Resources> CreateResources(const VkExtent2D &p_Extent, F &&p_CreateImageData,
                                      u32 p_FrameBufferLayers = 1) noexcept
    {
        if (m_Info.ImageCount == 0)
            return Result<Resources>::Error(VK_ERROR_INITIALIZATION_FAILED,
                                            "Image count must be greater than 0 to create resources");
        if (!m_Info.Allocator)
            return Result<Resources>::Error(VK_ERROR_INITIALIZATION_FAILED,
                                            "An allocator must be set to create resources");

        Resources resources;
        resources.m_Device = m_Device;
        resources.m_Allocator = m_Info.Allocator;

        TKit::StaticArray16<VkImageView> attachments{m_Info.Attachments.size(), VK_NULL_HANDLE};
        for (u32 i = 0; i < m_Info.ImageCount; ++i)
        {
            for (u32 j = 0; j < attachments.size(); ++j)
            {
                const auto imresult = std::forward<F>(p_CreateImageData)(i, j);
                if (!imresult)
                {
                    resources.Destroy();
                    return Result<Resources>::Error(imresult.GetError());
                }

                const ImageData &imageData = imresult.GetValue();
                resources.m_Images.push_back(imageData);
                attachments[j] = imageData.ImageView;
            }
            VkFramebufferCreateInfo frameBufferInfo{};
            frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frameBufferInfo.renderPass = m_RenderPass;
            frameBufferInfo.attachmentCount = attachments.size();
            frameBufferInfo.pAttachments = attachments.data();
            frameBufferInfo.width = p_Extent.width;
            frameBufferInfo.height = p_Extent.height;
            frameBufferInfo.layers = p_FrameBufferLayers;

            VkFramebuffer frameBuffer;
            const VkResult result =
                vkCreateFramebuffer(m_Device, &frameBufferInfo, m_Device.AllocationCallbacks, &frameBuffer);
            if (result != VK_SUCCESS)
            {
                resources.Destroy();
                return Result<Resources>::Error(result, "Failed to create the frame buffer");
            }

            resources.m_FrameBuffers.push_back(frameBuffer);
        }

        return Result<Resources>::Ok(resources);
    }

    /**
     * @brief Creates image data for a render pass attachment.
     *
     * Generates Vulkan image handles, views, and allocations for an attachment, based on the provided image
     * configuration.
     *
     * @param p_Info The Vulkan image creation info.
     * @param p_Range The subresource range for the image.
     * @param p_ViewType The type of image view to create.
     * @return A `Result` containing the created `ImageData` or an error.
     */
    Result<ImageData> CreateImageData(const VkImageCreateInfo &p_Info, const VkImageSubresourceRange &p_Range,
                                      VkImageViewType p_ViewType) const noexcept;

    /**
     * @brief Creates image data for a render pass attachment.
     *
     * Generates Vulkan image handles, views, and allocations for an attachment, based on the provided image
     * configuration.
     *
     * The view type is determined based on the image type in the image creation info.
     *
     * @param p_Info The Vulkan image creation info.
     * @param p_Range The subresource range for the image.
     * @return A `Result` containing the created `ImageData` or an error.
     */
    Result<ImageData> CreateImageData(const VkImageCreateInfo &p_Info,
                                      const VkImageSubresourceRange &p_Range) const noexcept;

    /**
     * @brief Creates image data for a render pass attachment.
     *
     * Generates Vulkan image handles, views, and allocations for an attachment, based on the provided image
     * configuration.
     *
     * The subresource range will default to the entire image.
     *
     * @param p_AttachmentIndex The index of the attachment.
     * @param p_Info The Vulkan image creation info.
     * @param p_ViewType The type of image view to create.
     * @return A `Result` containing the created `ImageData` or an error.
     */
    Result<ImageData> CreateImageData(u32 p_AttachmentIndex, const VkImageCreateInfo &p_Info,
                                      VkImageViewType p_ViewType) const noexcept;

    /**
     * @brief Creates image data for a render pass attachment.
     *
     * Generates Vulkan image handles, views, and allocations for an attachment, based on the provided image
     * configuration.
     *
     * The view type is determined based on the image type in the image creation info.
     * The subresource range will default to the entire image.
     *
     * @param p_AttachmentIndex The index of the attachment.
     * @param p_Info The Vulkan image creation info.
     * @return A `Result` containing the created `ImageData` or an error.
     */
    Result<ImageData> CreateImageData(u32 p_AttachmentIndex, const VkImageCreateInfo &p_Info) const noexcept;

    /**
     * @brief Creates image data for a render pass attachment.
     *
     * Generates Vulkan image handles, views, and allocations for an attachment, based on the provided image
     * configuration.
     *
     * The image creation info will default to the attachment's format and usage flags to provide a
     * basic image resource that works for most cases.
     *
     * @param p_AttachmentIndex The index of the attachment.
     * @param p_Extent The extent of the image.
     * @return A `Result` containing the created `ImageData` or an error.
     */
    Result<ImageData> CreateImageData(u32 p_AttachmentIndex, const VkExtent2D &p_Extent) const noexcept;

    /**
     * @brief Creates image data for a render pass attachment.
     *
     * A dummy method used when the user provides the image data directly. Commonly used when the image is provided by
     * a `SwapChain`.
     *
     * The underlying Resources struct will not take ownership of the image data (will skip this resource when the
     * `Destroy()` method is called), and the user is responsible for managing the image and image view (which will very
     * likely be automatically handled by the `SwapChain` itself).
     *
     * @param p_ImageView The image view to use.
     * @return A `Result` containing the created `ImageData` or an error.
     */
    Result<ImageData> CreateImageData(VkImageView p_ImageView) const noexcept;

    const Info &GetInfo() const noexcept;

    VkRenderPass GetRenderPass() const noexcept;

    explicit(false) operator VkRenderPass() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    void destroy() const noexcept;

    LogicalDevice::Proxy m_Device{};
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit