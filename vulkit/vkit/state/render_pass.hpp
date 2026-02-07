#pragma once

#ifndef VKIT_ENABLE_RENDER_PASS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_RENDER_PASS"
#endif

#include "vkit/resource/device_image.hpp"

namespace VKit
{
class RenderPass
{
  public:
    struct Attachment
    {
        VkAttachmentDescription Description{};
        DeviceImageFlags Flags;
    };

    class Builder;

  private:
    class AttachmentBuilder
    {
      public:
        AttachmentBuilder(Builder *builder, DeviceImageFlags flags);

        AttachmentBuilder &SetLoadOperation(VkAttachmentLoadOp operation,
                                            VkAttachmentLoadOp stencilOperation = VK_ATTACHMENT_LOAD_OP_MAX_ENUM);
        AttachmentBuilder &SetStoreOperation(VkAttachmentStoreOp operation,
                                             VkAttachmentStoreOp stencilOperation = VK_ATTACHMENT_STORE_OP_MAX_ENUM);

        AttachmentBuilder &SetStencilLoadOperation(VkAttachmentLoadOp operation);
        AttachmentBuilder &SetStencilStoreOperation(VkAttachmentStoreOp operation);

        AttachmentBuilder &RequestFormat(VkFormat format);
        AttachmentBuilder &AllowFormat(VkFormat format);

        AttachmentBuilder &SetLayouts(VkImageLayout initialLayout, VkImageLayout finalLayout);
        AttachmentBuilder &SetInitialLayout(VkImageLayout layout);
        AttachmentBuilder &SetFinalLayout(VkImageLayout layout);

        AttachmentBuilder &SetSampleCount(VkSampleCountFlagBits sampleCount);
        AttachmentBuilder &SetFlags(VkAttachmentDescriptionFlags flags);

        Builder &EndAttachment();

      private:
        Builder *m_Builder;
        Attachment m_Attachment{};
        TKit::TierArray<VkFormat> m_Formats;

        friend class Builder;
    };

    class SubpassBuilder
    {
      public:
        SubpassBuilder(Builder *builder, VkPipelineBindPoint bindPoint);

        SubpassBuilder &AddColorAttachment(u32 attachmentIndex,
                                           VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                           u32 resolveIndex = TKIT_U32_MAX);
        SubpassBuilder &AddColorAttachment(u32 attachmentIndex, u32 resolveIndex);

        SubpassBuilder &AddInputAttachment(u32 attachmentIndex, VkImageLayout layout);
        SubpassBuilder &AddPreserveAttachment(u32 attachmentIndex);

        SubpassBuilder &SetDepthStencilAttachment(u32 attachmentIndex,
                                                  VkImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        SubpassBuilder &SetFlags(VkSubpassDescriptionFlags flags);

        Builder &EndSubpass();

      private:
        Builder *m_Builder;
        VkSubpassDescription m_Description{};
        TKit::TierArray<VkAttachmentReference> m_ColorAttachments{};
        TKit::TierArray<VkAttachmentReference> m_InputAttachments{};
        TKit::TierArray<u32> m_PreserveAttachments{};
        TKit::TierArray<VkAttachmentReference> m_ResolveAttachments{};
        VkAttachmentReference m_DepthStencilAttachment{};

        friend class Builder;
    };

    class DependencyBuilder
    {
      public:
        DependencyBuilder(Builder *builder, u32 sourceSubpass, u32 destinationSubpass);

        DependencyBuilder &SetStageMask(VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);
        DependencyBuilder &SetAccessMask(VkAccessFlags sourceAccess, VkAccessFlags destinationAccess);

        DependencyBuilder &SetFlags(VkDependencyFlags flags);

        Builder &EndDependency();

      private:
        Builder *m_Builder;
        VkSubpassDependency m_Dependency{};

        friend class Builder;
    };

  public:
    class Builder
    {
      public:
        Builder(const LogicalDevice *device, const u32 imageCount) : m_Device(device), m_ImageCount(imageCount)
        {
        }

        VKIT_NO_DISCARD Result<RenderPass> Build() const;

        AttachmentBuilder &BeginAttachment(DeviceImageFlags flags);

        SubpassBuilder &BeginSubpass(VkPipelineBindPoint bindPoint);
        DependencyBuilder &BeginDependency(u32 sourceSubpass, u32 destinationSubpass);

        Builder &SetFlags(VkRenderPassCreateFlags flags);
        Builder &AddFlags(VkRenderPassCreateFlags flags);
        Builder &RemoveFlags(VkRenderPassCreateFlags flags);

        Builder &SetAllocator(VmaAllocator allocator);

      private:
        const LogicalDevice *m_Device;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VkRenderPassCreateFlags m_Flags = 0;
        u32 m_ImageCount;

        TKit::TierArray<AttachmentBuilder> m_Attachments{};
        TKit::TierArray<SubpassBuilder> m_Subpasses{};
        TKit::TierArray<DependencyBuilder> m_Dependencies{};
    };

    class Resources
    {
      public:
        void Destroy();

        VkImageView GetImageView(const u32 imageIndex, const u32 attachmentIndex) const
        {
            const u32 attachmentCount = m_Images.GetSize() / m_FrameBuffers.GetSize();
            return m_Images[imageIndex * attachmentCount + attachmentIndex].GetImageView();
        }
        VkFramebuffer GetFrameBuffer(const u32 imageIndex) const
        {
            return m_FrameBuffers[imageIndex];
        }

      private:
        ProxyDevice m_Device;
        TKit::TierArray<DeviceImage> m_Images;         // size: m_ImageCount * m_Attachments.GetSize()
        TKit::TierArray<VkFramebuffer> m_FrameBuffers; // size: m_ImageCount

        friend class RenderPass;
    };

    struct Info
    {
        VmaAllocator Allocator;
        TKit::TierArray<Attachment> Attachments;
        u32 ImageCount;
    };

    RenderPass() = default;
    RenderPass(const ProxyDevice &device, const VkRenderPass renderPass, const Info &info)
        : m_Device(device), m_RenderPass(renderPass), m_Info(info)
    {
    }

    void Destroy();

    /**
     * @brief Creates resources for the render pass, including frame buffers and image data.
     *
     * Populates frame buffers and associated images based on the provided extent and a user-defined image creation
     * callback. The `RenderPass` class provides many high-level options for `ImageData` struct creation, including the
     * case where the underlying resource is directly provided by a `SwapChain` image. See the
     * `ImageFactory::CreateImage()` methods for more.
     *
     * @tparam F The type of the callback function used for creating image data.
     * @param extent The dimensions of the frame buffer.
     * @param createImageData A callback function that generates image data for each attachment. Takes the image index
     * and attachment index as arguments.
     * @param frameBufferLayers The number of layers for each frame buffer (default: 1).
     * @return A `Result` containing the created `Resources` or an error.
     */
    template <typename F>
    VKIT_NO_DISCARD Result<Resources> CreateResources(const VkExtent2D &extent, F &&createImageData,
                                                      const u32 frameBufferLayers = 1)
    {
        Resources resources;
        resources.m_Device = m_Device;

        TKit::TierArray<VkImageView> attachments{m_Info.Attachments.GetSize()};
        for (u32 i = 0; i < m_Info.ImageCount; ++i)
        {
            for (u32 j = 0; j < attachments.GetSize(); ++j)
            {
                const auto imresult = std::forward<F>(createImageData)(i, j);
                if (!imresult)
                {
                    resources.Destroy();
                    attachments[j] = VK_NULL_HANDLE;
                    return imresult;
                }

                const DeviceImage &imageData = imresult.GetValue();
                resources.m_Images.Append(imageData);
                attachments[j] = imageData.GetImageView();
            }
            VkFramebufferCreateInfo frameBufferInfo{};
            frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frameBufferInfo.renderPass = m_RenderPass;
            frameBufferInfo.attachmentCount = attachments.GetSize();
            frameBufferInfo.pAttachments = attachments.GetData();
            frameBufferInfo.width = extent.width;
            frameBufferInfo.height = extent.height;
            frameBufferInfo.layers = frameBufferLayers;

            VkFramebuffer frameBuffer;
            VKIT_RETURN_IF_FAILED(m_Device.Table->CreateFramebuffer(m_Device, &frameBufferInfo,
                                                                    m_Device.AllocationCallbacks, &frameBuffer),
                                  Result<Resources>, resources.Destroy());

            resources.m_FrameBuffers.Append(frameBuffer);
        }

        return resources;
    }

    const Attachment &GetAttachment(const u32 attachmentIndex) const
    {
        return m_Info.Attachments[attachmentIndex];
    }
    const Info &GetInfo() const
    {
        return m_Info;
    }
    const ProxyDevice &GetDevice() const
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
    ProxyDevice m_Device{};
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
