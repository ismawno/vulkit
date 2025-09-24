#pragma once

#ifndef VKIT_ENABLE_RENDER_PASS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_RENDER_PASS"
#endif

#include "vkit/rendering/image.hpp"
#include "tkit/container/static_array.hpp"

namespace VKit
{
/**
 * @brief Represents a Vulkan render pass and its associated resources.
 *
 * Manages the configuration, creation, and destruction of Vulkan render passes, along with helper utilities for
 * attachments, subpasses, and dependencies. Also supports resource creation for associated frame buffers and images.
 */
class VKIT_API RenderPass
{
  public:
    struct Attachment
    {
        VkAttachmentDescription Description{};
        AttachmentFlags Flags;
    };

    class Builder;

  private:
    class AttachmentBuilder
    {
      public:
        AttachmentBuilder(Builder *p_Builder, AttachmentFlags p_Flags);

        AttachmentBuilder &SetLoadOperation(VkAttachmentLoadOp p_Operation,
                                            VkAttachmentLoadOp p_StencilOperation = VK_ATTACHMENT_LOAD_OP_MAX_ENUM);
        AttachmentBuilder &SetStoreOperation(VkAttachmentStoreOp p_Operation,
                                             VkAttachmentStoreOp p_StencilOperation = VK_ATTACHMENT_STORE_OP_MAX_ENUM);

        AttachmentBuilder &SetStencilLoadOperation(VkAttachmentLoadOp p_Operation);
        AttachmentBuilder &SetStencilStoreOperation(VkAttachmentStoreOp p_Operation);

        AttachmentBuilder &RequestFormat(VkFormat p_Format);
        AttachmentBuilder &AllowFormat(VkFormat p_Format);

        AttachmentBuilder &SetLayouts(VkImageLayout p_InitialLayout, VkImageLayout p_FinalLayout);
        AttachmentBuilder &SetInitialLayout(VkImageLayout p_Layout);
        AttachmentBuilder &SetFinalLayout(VkImageLayout p_Layout);

        AttachmentBuilder &SetSampleCount(VkSampleCountFlagBits p_SampleCount);
        AttachmentBuilder &SetFlags(VkAttachmentDescriptionFlags p_Flags);

        Builder &EndAttachment();

      private:
        Builder *m_Builder;
        Attachment m_Attachment{};
        TKit::StaticArray16<VkFormat> m_Formats;

        friend class Builder;
    };

    class SubpassBuilder
    {
      public:
        SubpassBuilder(Builder *p_Builder, VkPipelineBindPoint p_BindPoint);

        SubpassBuilder &AddColorAttachment(u32 p_AttachmentIndex,
                                           VkImageLayout p_Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                           u32 p_ResolveIndex = UINT32_MAX);
        SubpassBuilder &AddColorAttachment(u32 p_AttachmentIndex, u32 p_ResolveIndex);

        SubpassBuilder &AddInputAttachment(u32 p_AttachmentIndex, VkImageLayout p_Layout);
        SubpassBuilder &AddPreserveAttachment(u32 p_AttachmentIndex);

        SubpassBuilder &SetDepthStencilAttachment(u32 p_AttachmentIndex,
                                                  VkImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        SubpassBuilder &SetFlags(VkSubpassDescriptionFlags p_Flags);

        Builder &EndSubpass();

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
        DependencyBuilder(Builder *p_Builder, u32 p_SourceSubpass, u32 p_DestinationSubpass);

        DependencyBuilder &SetStageMask(VkPipelineStageFlags p_SourceStage, VkPipelineStageFlags p_DestinationStage);
        DependencyBuilder &SetAccessMask(VkAccessFlags p_SourceAccess, VkAccessFlags p_DestinationAccess);

        DependencyBuilder &SetFlags(VkDependencyFlags p_Flags);

        Builder &EndDependency();

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
        Builder(const LogicalDevice *p_Device, const u32 p_ImageCount) : m_Device(p_Device), m_ImageCount(p_ImageCount)
        {
        }

        Result<RenderPass> Build() const;

        AttachmentBuilder &BeginAttachment(AttachmentFlags p_Flags);

        SubpassBuilder &BeginSubpass(VkPipelineBindPoint p_BindPoint);
        DependencyBuilder &BeginDependency(u32 p_SourceSubpass, u32 p_DestinationSubpass);

        Builder &SetFlags(VkRenderPassCreateFlags p_Flags);
        Builder &AddFlags(VkRenderPassCreateFlags p_Flags);
        Builder &RemoveFlags(VkRenderPassCreateFlags p_Flags);

        Builder &SetAllocator(VmaAllocator p_Allocator);

      private:
        const LogicalDevice *m_Device;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VkRenderPassCreateFlags m_Flags = 0;
        u32 m_ImageCount;

        TKit::StaticArray16<AttachmentBuilder> m_Attachments{};
        TKit::StaticArray8<SubpassBuilder> m_Subpasses{};
        TKit::StaticArray8<DependencyBuilder> m_Dependencies{};
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
        void Destroy();
        void SubmitForDeletion(DeletionQueue &p_Queue) const;

        VkImageView GetImageView(const u32 p_ImageIndex, const u32 p_AttachmentIndex) const
        {
            const u32 attachmentCount = m_Images.GetSize() / m_FrameBuffers.GetSize();
            return m_Images[p_ImageIndex * attachmentCount + p_AttachmentIndex].ImageView;
        }
        VkFramebuffer GetFrameBuffer(const u32 p_ImageIndex) const
        {
            return m_FrameBuffers[p_ImageIndex];
        }

      private:
        void destroy() const;

        ImageHouse m_ImageHouse;
        TKit::StaticArray64<Image> m_Images;              // size: m_ImageCount * m_Attachments.GetSize()
        TKit::StaticArray4<VkFramebuffer> m_FrameBuffers; // size: m_ImageCount

        friend class RenderPass;
    };

    struct Info
    {
        VmaAllocator Allocator;
        TKit::StaticArray16<Attachment> Attachments;
        u32 ImageCount;
    };

    RenderPass() = default;
    RenderPass(const LogicalDevice::Proxy &p_Device, const VkRenderPass p_RenderPass, const Info &p_Info)
        : m_Device(p_Device), m_RenderPass(p_RenderPass), m_Info(p_Info)
    {
    }

    void Destroy();
    void SubmitForDeletion(DeletionQueue &p_Queue) const;

    /**
     * @brief Creates resources for the render pass, including frame buffers and image data.
     *
     * Populates frame buffers and associated images based on the provided extent and a user-defined image creation
     * callback. The `RenderPass` class provides many high-level options for `ImageData` struct creation, including the
     * case where the underlying resource is directly provided by a `SwapChain` image. See the
     * `ImageHouse::CreateImage()` methods for more.
     *
     * @tparam F The type of the callback function used for creating image data.
     * @param p_Extent The dimensions of the frame buffer.
     * @param p_CreateImageData A callback function that generates image data for each attachment. Takes the image index
     * and attachment index as arguments.
     * @param p_FrameBufferLayers The number of layers for each frame buffer (default: 1).
     * @return A `Result` containing the created `Resources` or an error.
     */
    template <typename F>
    Result<Resources> CreateResources(const VkExtent2D &p_Extent, F &&p_CreateImageData, u32 p_FrameBufferLayers = 1)
    {
        if (m_Info.ImageCount == 0)
            return Result<Resources>::Error(VK_ERROR_INITIALIZATION_FAILED,
                                            "Image count must be greater than 0 to create resources");
        if (!m_Info.Allocator)
            return Result<Resources>::Error(VK_ERROR_INITIALIZATION_FAILED,
                                            "An allocator must be set to create resources");

        Resources resources;
        const auto result = ImageHouse::Create(m_Device, m_Info.Allocator);
        if (!result)
            return Result<Resources>::Error(result.GetError());

        resources.m_ImageHouse = result.GetValue();

        VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCreateFramebuffer, Result<Resources>);

        TKit::StaticArray16<VkImageView> attachments{m_Info.Attachments.GetSize(), VK_NULL_HANDLE};
        for (u32 i = 0; i < m_Info.ImageCount; ++i)
        {
            for (u32 j = 0; j < attachments.GetSize(); ++j)
            {
                const auto imresult = std::forward<F>(p_CreateImageData)(resources.m_ImageHouse, i, j);
                if (!imresult)
                {
                    resources.Destroy();
                    return Result<Resources>::Error(imresult.GetError());
                }

                const Image &imageData = imresult.GetValue();
                resources.m_Images.Append(imageData);
                attachments[j] = imageData.ImageView;
            }
            VkFramebufferCreateInfo frameBufferInfo{};
            frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frameBufferInfo.renderPass = m_RenderPass;
            frameBufferInfo.attachmentCount = attachments.GetSize();
            frameBufferInfo.pAttachments = attachments.GetData();
            frameBufferInfo.width = p_Extent.width;
            frameBufferInfo.height = p_Extent.height;
            frameBufferInfo.layers = p_FrameBufferLayers;

            VkFramebuffer frameBuffer;
            const VkResult result = m_Device.Table->CreateFramebuffer(m_Device, &frameBufferInfo,
                                                                      m_Device.AllocationCallbacks, &frameBuffer);
            if (result != VK_SUCCESS)
            {
                resources.Destroy();
                return Result<Resources>::Error(result, "Failed to create the frame buffer");
            }

            resources.m_FrameBuffers.Append(frameBuffer);
        }

        return Result<Resources>::Ok(resources);
    }

    const Attachment &GetAttachment(const u32 p_AttachmentIndex) const
    {
        return m_Info.Attachments[p_AttachmentIndex];
    }
    const Info &GetInfo() const
    {
        return m_Info;
    }
    const LogicalDevice::Proxy &GetDevice() const
    {
        return m_Device;
    }
    VkRenderPass GetHandle() const
    {
        return m_RenderPass;
    }
    operator VkRenderPass() const
    {
        return m_RenderPass;
    }
    operator bool() const
    {
        return m_RenderPass != VK_NULL_HANDLE;
    }

  private:
    void destroy() const;

    LogicalDevice::Proxy m_Device{};
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
