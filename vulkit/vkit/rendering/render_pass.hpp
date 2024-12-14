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
        DynamicArray<VkFormat> m_Formats;

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
        DynamicArray<VkAttachmentReference> m_ColorAttachments{};
        DynamicArray<VkAttachmentReference> m_InputAttachments{};
        DynamicArray<u32> m_PreserveAttachments{};
        DynamicArray<VkAttachmentReference> m_ResolveAttachments{};
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

        DynamicArray<AttachmentBuilder> m_Attachments{};
        DynamicArray<SubpassBuilder> m_Subpasses{};
        DynamicArray<DependencyBuilder> m_Dependencies{};
    };

    struct ImageData
    {
        VkImage Image;
        VkImageView ImageView;
        VmaAllocation Allocation;
    };

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

        DynamicArray<ImageData> m_Images;           // size: m_ImageCount * m_Attachments.size()
        DynamicArray<VkFramebuffer> m_FrameBuffers; // size: m_ImageCount

        friend class RenderPass;
    };

    struct Info
    {
        VmaAllocator Allocator;
        DynamicArray<Attachment> Attachments;
        u32 ImageCount;
    };

    RenderPass() noexcept = default;
    RenderPass(const LogicalDevice::Proxy &p_Device, VkRenderPass p_RenderPass, const Info &p_Info) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

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
        resources.m_Images.reserve(m_Info.ImageCount * m_Info.Attachments.size());
        resources.m_FrameBuffers.reserve(m_Info.ImageCount);

        DynamicArray<VkImageView> attachments{m_Info.Attachments.size(), VK_NULL_HANDLE};
        const u32 attachmentCount = static_cast<u32>(attachments.size());

        for (u32 i = 0; i < m_Info.ImageCount; ++i)
        {
            for (u32 j = 0; j < attachmentCount; ++j)
            {
                const auto imresult = p_CreateImageData(i, j);
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
            frameBufferInfo.attachmentCount = static_cast<u32>(attachments.size());
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

    Result<ImageData> CreateImageData(const VkImageCreateInfo &p_Info, const VkImageSubresourceRange &p_Range,
                                      VkImageViewType p_ViewType) const noexcept;
    Result<ImageData> CreateImageData(const VkImageCreateInfo &p_Info,
                                      const VkImageSubresourceRange &p_Range) const noexcept;
    Result<ImageData> CreateImageData(u32 p_AttachmentIndex, const VkImageCreateInfo &p_Info,
                                      VkImageViewType p_ViewType) const noexcept;
    Result<ImageData> CreateImageData(u32 p_AttachmentIndex, const VkImageCreateInfo &p_Info) const noexcept;

    Result<ImageData> CreateImageData(u32 p_AttachmentIndex, const VkExtent2D &p_Extent) const noexcept;
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