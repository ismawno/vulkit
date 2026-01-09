#pragma once

#ifndef VKIT_ENABLE_RENDER_PASS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_RENDER_PASS"
#endif

#include "vkit/resource/image.hpp"
#include "tkit/container/array.hpp"

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
        AttachmentBuilder(Builder *p_Builder, DeviceImageFlags p_Flags);

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
        TKit::Array16<VkFormat> m_Formats;

        friend class Builder;
    };

    class SubpassBuilder
    {
      public:
        SubpassBuilder(Builder *p_Builder, VkPipelineBindPoint p_BindPoint);

        SubpassBuilder &AddColorAttachment(u32 p_AttachmentIndex,
                                           VkImageLayout p_Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                           u32 p_ResolveIndex = TKIT_U32_MAX);
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
        TKit::Array8<VkAttachmentReference> m_ColorAttachments{};
        TKit::Array8<VkAttachmentReference> m_InputAttachments{};
        TKit::Array8<u32> m_PreserveAttachments{};
        TKit::Array8<VkAttachmentReference> m_ResolveAttachments{};
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
    class Builder
    {
      public:
        Builder(const LogicalDevice *p_Device, const u32 p_ImageCount) : m_Device(p_Device), m_ImageCount(p_ImageCount)
        {
        }

        VKIT_NO_DISCARD Result<RenderPass> Build() const;

        AttachmentBuilder &BeginAttachment(DeviceImageFlags p_Flags);

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

        TKit::Array16<AttachmentBuilder> m_Attachments{};
        TKit::Array8<SubpassBuilder> m_Subpasses{};
        TKit::Array8<DependencyBuilder> m_Dependencies{};
    };

    class Resources
    {
      public:
        void Destroy();

        VkImageView GetImageView(const u32 p_ImageIndex, const u32 p_AttachmentIndex) const
        {
            const u32 attachmentCount = m_Images.GetSize() / m_FrameBuffers.GetSize();
            return m_Images[p_ImageIndex * attachmentCount + p_AttachmentIndex].GetImageView();
        }
        VkFramebuffer GetFrameBuffer(const u32 p_ImageIndex) const
        {
            return m_FrameBuffers[p_ImageIndex];
        }

      private:
        ProxyDevice m_Device;
        TKit::Array128<DeviceImage> m_Images;       // size: m_ImageCount * m_Attachments.GetSize()
        TKit::Array8<VkFramebuffer> m_FrameBuffers; // size: m_ImageCount

        friend class RenderPass;
    };

    struct Info
    {
        VmaAllocator Allocator;
        TKit::Array16<Attachment> Attachments;
        u32 ImageCount;
    };

    RenderPass() = default;
    RenderPass(const ProxyDevice &p_Device, const VkRenderPass p_RenderPass, const Info &p_Info)
        : m_Device(p_Device), m_RenderPass(p_RenderPass), m_Info(p_Info)
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
     * @param p_Extent The dimensions of the frame buffer.
     * @param p_CreateImageData A callback function that generates image data for each attachment. Takes the image index
     * and attachment index as arguments.
     * @param p_FrameBufferLayers The number of layers for each frame buffer (default: 1).
     * @return A `Result` containing the created `Resources` or an error.
     */
    template <typename F>
    VKIT_NO_DISCARD Result<Resources> CreateResources(const VkExtent2D &p_Extent, F &&p_CreateImageData,
                                                      u32 p_FrameBufferLayers = 1)
    {
        Resources resources;
        resources.m_Device = m_Device;

        TKit::Array16<VkImageView> attachments{m_Info.Attachments.GetSize(), VK_NULL_HANDLE};
        for (u32 i = 0; i < m_Info.ImageCount; ++i)
        {
            for (u32 j = 0; j < attachments.GetSize(); ++j)
            {
                const auto imresult = std::forward<F>(p_CreateImageData)(i, j);
                if (!imresult)
                {
                    resources.Destroy();
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
            frameBufferInfo.width = p_Extent.width;
            frameBufferInfo.height = p_Extent.height;
            frameBufferInfo.layers = p_FrameBufferLayers;

            VkFramebuffer frameBuffer;
            const VkResult result = m_Device.Table->CreateFramebuffer(m_Device, &frameBufferInfo,
                                                                      m_Device.AllocationCallbacks, &frameBuffer);
            if (result != VK_SUCCESS)
            {
                resources.Destroy();
                return Result<Resources>::Error(result);
            }

            resources.m_FrameBuffers.Append(frameBuffer);
        }

        return resources;
    }

    const Attachment &GetAttachment(const u32 p_AttachmentIndex) const
    {
        return m_Info.Attachments[p_AttachmentIndex];
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
