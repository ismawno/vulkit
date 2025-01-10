#include "vkit/core/pch.hpp"
#include "vkit/rendering/render_pass.hpp"

namespace VKit
{
RenderPass::Builder::Builder(const LogicalDevice *p_Device, const u32 p_ImageCount) noexcept
    : m_Device(p_Device), m_ImageCount(p_ImageCount)
{
}

Result<RenderPass> RenderPass::Builder::Build() const noexcept
{
    if (m_Subpasses.empty())
        return Result<RenderPass>::Error(VK_ERROR_INITIALIZATION_FAILED, "Render must have at least one subpass");

    TKit::StaticArray16<Attachment> attachments;
    TKit::StaticArray16<VkAttachmentDescription> attDescriptions;
    for (const AttachmentBuilder &attachment : m_Attachments)
    {
        TKit::StaticArray16<VkFormat> formats = attachment.m_Formats;
        if (formats.empty())
        {
            if (attachment.m_Attachment.TypeFlags & Attachment::Flag_Color)
                formats.push_back(VK_FORMAT_B8G8R8A8_SRGB);
            else if ((attachment.m_Attachment.TypeFlags & Attachment::Flag_Depth) &&
                     (attachment.m_Attachment.TypeFlags & Attachment::Flag_Stencil))
                formats.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
            else if (attachment.m_Attachment.TypeFlags & Attachment::Flag_Depth)
                formats.push_back(VK_FORMAT_D32_SFLOAT);
            else if (attachment.m_Attachment.TypeFlags & Attachment::Flag_Stencil)
                formats.push_back(VK_FORMAT_S8_UINT);
        }
        VkFormatFeatureFlags flags = 0;
        if (attachment.m_Attachment.TypeFlags & Attachment::Flag_Color)
            flags = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
        else if ((attachment.m_Attachment.TypeFlags & Attachment::Flag_Depth) ||
                 (attachment.m_Attachment.TypeFlags & Attachment::Flag_Stencil))
            flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

        const auto result = m_Device->FindSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL, flags);
        if (!result)
            return Result<RenderPass>::Error(result.GetError());

        Attachment att = attachment.m_Attachment;
        att.Description.format = result.GetValue();

        attachments.push_back(att);
        attDescriptions.push_back(att.Description);
    }

    TKit::StaticArray8<VkSubpassDescription> subpasses;
    for (const SubpassBuilder &subpass : m_Subpasses)
        subpasses.push_back(subpass.m_Description);

    TKit::StaticArray8<VkSubpassDependency> dependencies;
    for (const DependencyBuilder &dependency : m_Dependencies)
        dependencies.push_back(dependency.m_Dependency);

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = attDescriptions.size();
    createInfo.pAttachments = attDescriptions.data();
    createInfo.subpassCount = subpasses.size();
    createInfo.pSubpasses = subpasses.data();
    createInfo.dependencyCount = dependencies.size();
    createInfo.pDependencies = dependencies.data();
    createInfo.flags = m_Flags;

    const LogicalDevice::Proxy proxy = m_Device->CreateProxy();

    VkRenderPass renderPass;
    const VkResult result = vkCreateRenderPass(proxy, &createInfo, proxy.AllocationCallbacks, &renderPass);
    if (result != VK_SUCCESS)
        return Result<RenderPass>::Error(result, "Failed to create render pass");

    RenderPass::Info info{};
    info.Allocator = m_Allocator;
    info.Attachments = attachments;
    info.ImageCount = m_ImageCount;

    return Result<RenderPass>::Ok(proxy, renderPass, info);
}

RenderPass::RenderPass(const LogicalDevice::Proxy &p_Device, const VkRenderPass p_RenderPass,
                       const Info &p_Info) noexcept
    : m_Device(p_Device), m_RenderPass(p_RenderPass), m_Info(p_Info)
{
}

void RenderPass::destroy() const noexcept
{
    TKIT_ASSERT(m_RenderPass, "[VULKIT] Render pass is already destroyed");
    vkDestroyRenderPass(m_Device, m_RenderPass, m_Device.AllocationCallbacks);
}

void RenderPass::Destroy() noexcept
{
    destroy();
    m_RenderPass = VK_NULL_HANDLE;
}
void RenderPass::SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
{
    const RenderPass renderPass = *this;
    p_Queue.Push([renderPass]() { renderPass.destroy(); });
}

Result<RenderPass::ImageData> RenderPass::CreateImageData(const VkImageCreateInfo &p_Info,
                                                          const VkImageSubresourceRange &p_Range,
                                                          const VkImageViewType p_ViewType) const noexcept
{
    if (!m_Info.Allocator)
        return Result<ImageData>::Error(VK_ERROR_INITIALIZATION_FAILED, "An allocator must be set to create resources");

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    ImageData imageData;
    VkResult result =
        vmaCreateImage(m_Info.Allocator, &p_Info, &allocInfo, &imageData.Image, &imageData.Allocation, nullptr);
    if (result != VK_SUCCESS)
        return Result<ImageData>::Error(result, "Failed to create image");

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = imageData.Image;
    viewInfo.viewType = p_ViewType;
    viewInfo.format = p_Info.format;
    viewInfo.subresourceRange = p_Range;

    result = vkCreateImageView(m_Device, &viewInfo, m_Device.AllocationCallbacks, &imageData.ImageView);
    if (result != VK_SUCCESS)
    {
        vmaDestroyImage(m_Info.Allocator, imageData.Image, imageData.Allocation);
        return Result<ImageData>::Error(result, "Failed to create image view");
    }
    return Result<ImageData>::Ok(imageData);
}
static VkImageViewType getImageViewType(const VkImageType p_Type) noexcept
{
    switch (p_Type)
    {
    case VK_IMAGE_TYPE_1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case VK_IMAGE_TYPE_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case VK_IMAGE_TYPE_3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    default:
        break;
    }
    return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
}
Result<RenderPass::ImageData> RenderPass::CreateImageData(const VkImageCreateInfo &p_Info,
                                                          const VkImageSubresourceRange &p_Range) const noexcept
{
    const VkImageViewType viewType = getImageViewType(p_Info.imageType);
    if (viewType == VK_IMAGE_VIEW_TYPE_MAX_ENUM)
        return Result<ImageData>::Error(VK_ERROR_INITIALIZATION_FAILED, "Invalid image type");
    return CreateImageData(p_Info, p_Range, viewType);
}
static Result<VkImageSubresourceRange> getRange(const VkImageCreateInfo &p_Info,
                                                const Attachment::Flags p_Flags) noexcept
{
    VkImageSubresourceRange range{};
    if (p_Flags & Attachment::Flag_Color)
        range.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
    else if (p_Flags & Attachment::Flag_Depth)
        range.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (p_Flags & Attachment::Flag_Stencil)
        range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    else
        return Result<VkImageSubresourceRange>::Error(VK_ERROR_INITIALIZATION_FAILED, "Invalid attachment type");

    range.baseMipLevel = 0;
    range.levelCount = p_Info.mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = p_Info.arrayLayers;
    return Result<VkImageSubresourceRange>::Ok(range);
}
Result<RenderPass::ImageData> RenderPass::CreateImageData(const u32 p_AttachmentIndex, const VkImageCreateInfo &p_Info,
                                                          const VkImageViewType p_ViewType) const noexcept
{
    const auto rangeResult = getRange(p_Info, m_Info.Attachments[p_AttachmentIndex].TypeFlags);
    if (!rangeResult)
        return Result<ImageData>::Error(rangeResult.GetError());

    const VkImageSubresourceRange &range = rangeResult.GetValue();
    return CreateImageData(p_Info, range, p_ViewType);
}
Result<RenderPass::ImageData> RenderPass::CreateImageData(const u32 p_AttachmentIndex,
                                                          const VkImageCreateInfo &p_Info) const noexcept
{
    const VkImageViewType viewType = getImageViewType(p_Info.imageType);
    if (viewType == VK_IMAGE_VIEW_TYPE_MAX_ENUM)
        return Result<ImageData>::Error(VK_ERROR_INITIALIZATION_FAILED, "Invalid image type");

    const auto rangeResult = getRange(p_Info, m_Info.Attachments[p_AttachmentIndex].TypeFlags);
    if (!rangeResult)
        return Result<ImageData>::Error(rangeResult.GetError());

    const VkImageSubresourceRange &range = rangeResult.GetValue();
    return CreateImageData(p_Info, range, viewType);
}
Result<RenderPass::ImageData> RenderPass::CreateImageData(const u32 p_AttachmentIndex,
                                                          const VkExtent2D &p_Extent) const noexcept
{
    const Attachment &attachment = m_Info.Attachments[p_AttachmentIndex];

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = p_Extent.width;
    imageInfo.extent.height = p_Extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = attachment.Description.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = 0;
    if (attachment.TypeFlags & Attachment::Flag_Color)
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else if ((attachment.TypeFlags & Attachment::Flag_Depth) || (attachment.TypeFlags & Attachment::Flag_Stencil))
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    if (attachment.TypeFlags & Attachment::Flag_Input)
        imageInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    if (attachment.TypeFlags & Attachment::Flag_Sampled)
        imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

    return CreateImageData(p_AttachmentIndex, imageInfo);
}
Result<RenderPass::ImageData> RenderPass::CreateImageData(const VkImageView p_ImageView) const noexcept
{
    ImageData imageData{};
    imageData.Image = VK_NULL_HANDLE;
    imageData.Allocation = VK_NULL_HANDLE;
    imageData.ImageView = p_ImageView;
    return Result<ImageData>::Ok(imageData);
}

const RenderPass::Info &RenderPass::GetInfo() const noexcept
{
    return m_Info;
}
VkRenderPass RenderPass::GetRenderPass() const noexcept
{
    return m_RenderPass;
}
RenderPass::operator VkRenderPass() const noexcept
{
    return m_RenderPass;
}
RenderPass::operator bool() const noexcept
{
    return m_RenderPass != VK_NULL_HANDLE;
}

void RenderPass::Resources::destroy() const noexcept
{
    for (const ImageData &data : m_Images)
        if (data.Image)
        {
            vmaDestroyImage(m_Allocator, data.Image, data.Allocation);
            vkDestroyImageView(m_Device, data.ImageView, m_Device.AllocationCallbacks);
        }

    for (const VkFramebuffer &frameBuffer : m_FrameBuffers)
        vkDestroyFramebuffer(m_Device, frameBuffer, m_Device.AllocationCallbacks);
}
void RenderPass::Resources::Destroy() noexcept
{
    destroy();
    m_Images.clear();
    m_FrameBuffers.clear();
}
void RenderPass::Resources::SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
{
    const Resources resources = *this;
    p_Queue.Push([resources]() { resources.destroy(); });
}

VkImageView RenderPass::Resources::GetImageView(const u32 p_ImageIndex, const u32 p_AttachmentIndex) const noexcept
{
    const u32 attachmentCount = m_Images.size() / m_FrameBuffers.size();
    return m_Images[p_ImageIndex * attachmentCount + p_AttachmentIndex].ImageView;
}
VkFramebuffer RenderPass::Resources::GetFrameBuffer(const u32 p_ImageIndex) const noexcept
{
    return m_FrameBuffers[p_ImageIndex];
}

RenderPass::AttachmentBuilder &RenderPass::Builder::BeginAttachment(const Attachment::Flags p_TypeFlags) noexcept
{
    return m_Attachments.emplace_back(this, p_TypeFlags);
}
RenderPass::SubpassBuilder &RenderPass::Builder::BeginSubpass(const VkPipelineBindPoint p_BindPoint) noexcept
{
    return m_Subpasses.emplace_back(this, p_BindPoint);
}
RenderPass::DependencyBuilder &RenderPass::Builder::BeginDependency(const u32 p_SourceSubpass,
                                                                    const u32 p_DestinationSubpass) noexcept
{
    return m_Dependencies.emplace_back(this, p_SourceSubpass, p_DestinationSubpass);
}
RenderPass::Builder &RenderPass::Builder::SetAllocator(const VmaAllocator p_Allocator) noexcept
{
    m_Allocator = p_Allocator;
    return *this;
}
RenderPass::Builder &RenderPass::Builder::SetFlags(const VkRenderPassCreateFlags p_Flags) noexcept
{
    m_Flags = p_Flags;
    return *this;
}
RenderPass::Builder &RenderPass::Builder::AddFlags(const VkRenderPassCreateFlags p_Flags) noexcept
{
    m_Flags |= p_Flags;
    return *this;
}
RenderPass::Builder &RenderPass::Builder::RemoveFlags(const VkRenderPassCreateFlags p_Flags) noexcept
{
    m_Flags &= ~p_Flags;
    return *this;
}

RenderPass::AttachmentBuilder::AttachmentBuilder(RenderPass::Builder *p_Builder,
                                                 const Attachment::Flags p_TypeFlags) noexcept
    : m_Builder(p_Builder)
{
    TKIT_ASSERT(p_TypeFlags, "[VULKIT] Attachment must have at least one type flag");
    TKIT_ASSERT(!((p_TypeFlags & Attachment::Flag_Color) && (p_TypeFlags & Attachment::Flag_Depth)),
                "[VULKIT] Attachment must be color or depth, not both");
    TKIT_ASSERT(!((p_TypeFlags & Attachment::Flag_Color) && (p_TypeFlags & Attachment::Flag_Stencil)),
                "[VULKIT] Attachment must be color or stencil, not both");

    m_Attachment.Description.samples = VK_SAMPLE_COUNT_1_BIT;
    m_Attachment.Description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_Attachment.Description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_Attachment.Description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_Attachment.Description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_Attachment.Description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_Attachment.Description.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_Attachment.Description.flags = 0;
    m_Attachment.TypeFlags = p_TypeFlags;

    if (p_TypeFlags & Attachment::Flag_Color)
    {
        m_Attachment.Description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_Attachment.Description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }
    if (p_TypeFlags & Attachment::Flag_Depth)
    {
        m_Attachment.Description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_Attachment.Description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    if (p_TypeFlags & Attachment::Flag_Stencil)
    {
        m_Attachment.Description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_Attachment.Description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
}

RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetLoadOperation(
    const VkAttachmentLoadOp p_Operation, const VkAttachmentLoadOp p_StencilOperation) noexcept
{
    m_Attachment.Description.loadOp = p_Operation;
    if (p_StencilOperation != VK_ATTACHMENT_LOAD_OP_MAX_ENUM)
        m_Attachment.Description.stencilLoadOp = p_StencilOperation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetStoreOperation(
    const VkAttachmentStoreOp p_Operation, const VkAttachmentStoreOp p_StencilOperation) noexcept
{
    m_Attachment.Description.storeOp = p_Operation;
    if (p_StencilOperation != VK_ATTACHMENT_STORE_OP_MAX_ENUM)
        m_Attachment.Description.stencilStoreOp = p_StencilOperation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetStencilLoadOperation(
    const VkAttachmentLoadOp p_Operation) noexcept
{
    m_Attachment.Description.stencilLoadOp = p_Operation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetStencilStoreOperation(
    const VkAttachmentStoreOp p_Operation) noexcept
{
    m_Attachment.Description.stencilStoreOp = p_Operation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::RequestFormat(const VkFormat p_Format) noexcept
{
    m_Formats.insert(m_Formats.begin(), p_Format);
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::AllowFormat(const VkFormat p_Format) noexcept
{
    m_Formats.push_back(p_Format);
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetLayouts(const VkImageLayout p_InitialLayout,
                                                                         const VkImageLayout p_FinalLayout) noexcept
{
    m_Attachment.Description.initialLayout = p_InitialLayout;
    m_Attachment.Description.finalLayout = p_FinalLayout;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetInitialLayout(const VkImageLayout p_Layout) noexcept
{
    m_Attachment.Description.initialLayout = p_Layout;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetFinalLayout(const VkImageLayout p_Layout) noexcept
{
    m_Attachment.Description.finalLayout = p_Layout;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetSampleCount(
    const VkSampleCountFlagBits p_Samples) noexcept
{
    m_Attachment.Description.samples = p_Samples;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetFlags(
    const VkAttachmentDescriptionFlags p_Flags) noexcept
{
    m_Attachment.Description.flags = p_Flags;
    return *this;
}
RenderPass::Builder &RenderPass::AttachmentBuilder::EndAttachment() noexcept
{
    return *m_Builder;
}

RenderPass::SubpassBuilder::SubpassBuilder(RenderPass::Builder *p_Builder,
                                           const VkPipelineBindPoint p_BindPoint) noexcept
    : m_Builder(p_Builder)
{
    m_Description.pipelineBindPoint = p_BindPoint;
    m_DepthStencilAttachment.attachment = UINT32_MAX;
}

RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddColorAttachment(const u32 p_AttachmentIndex,
                                                                           const VkImageLayout p_Layout,
                                                                           const u32 p_ResolveIndex) noexcept
{
    m_ColorAttachments.push_back(VkAttachmentReference{p_AttachmentIndex, p_Layout});
    if (p_ResolveIndex != UINT32_MAX)
    {
        m_ResolveAttachments.push_back(VkAttachmentReference{p_ResolveIndex, p_Layout});
        TKIT_ASSERT(m_ResolveAttachments.size() == m_ColorAttachments.size(),
                    "[VULKIT] Mismatched color and resolve attachments");
    }
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddColorAttachment(const u32 p_AttachmentIndex,
                                                                           const u32 p_ResolveIndex) noexcept
{
    return AddColorAttachment(p_AttachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, p_ResolveIndex);
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddInputAttachment(const u32 p_AttachmentIndex,
                                                                           const VkImageLayout p_Layout) noexcept
{
    m_InputAttachments.push_back(VkAttachmentReference{p_AttachmentIndex, p_Layout});
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddPreserveAttachment(const u32 p_AttachmentIndex) noexcept
{
    m_PreserveAttachments.push_back(p_AttachmentIndex);
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::SetDepthStencilAttachment(const u32 p_AttachmentIndex,
                                                                                  const VkImageLayout p_Layout) noexcept
{
    m_DepthStencilAttachment = {p_AttachmentIndex, p_Layout};
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::SetFlags(const VkSubpassDescriptionFlags p_Flags) noexcept
{
    m_Description.flags = p_Flags;
    return *this;
}
RenderPass::Builder &RenderPass::SubpassBuilder::EndSubpass() noexcept
{
    m_Description.colorAttachmentCount = m_ColorAttachments.size();
    m_Description.pColorAttachments = m_ColorAttachments.empty() ? nullptr : m_ColorAttachments.data();
    m_Description.inputAttachmentCount = m_InputAttachments.size();
    m_Description.pInputAttachments = m_InputAttachments.empty() ? nullptr : m_InputAttachments.data();
    m_Description.preserveAttachmentCount = m_PreserveAttachments.size();
    m_Description.pPreserveAttachments = m_PreserveAttachments.empty() ? nullptr : m_PreserveAttachments.data();
    m_Description.pResolveAttachments = m_ResolveAttachments.empty() ? nullptr : m_ResolveAttachments.data();
    m_Description.pDepthStencilAttachment =
        m_DepthStencilAttachment.attachment == UINT32_MAX ? nullptr : &m_DepthStencilAttachment;

    return *m_Builder;
}

RenderPass::DependencyBuilder::DependencyBuilder(RenderPass::Builder *p_Builder, const u32 p_SourceSubpass,
                                                 const u32 p_DestinationSubpass) noexcept
    : m_Builder(p_Builder)
{
    m_Dependency.srcSubpass = p_SourceSubpass;
    m_Dependency.dstSubpass = p_DestinationSubpass;
}

RenderPass::DependencyBuilder &RenderPass::DependencyBuilder::SetStageMask(
    const VkPipelineStageFlags p_SourceStage, const VkPipelineStageFlags p_DestinationStage) noexcept
{
    m_Dependency.srcStageMask = p_SourceStage;
    m_Dependency.dstStageMask = p_DestinationStage;
    return *this;
}
RenderPass::DependencyBuilder &RenderPass::DependencyBuilder::SetAccessMask(
    const VkAccessFlags p_SourceAccess, const VkAccessFlags p_DestinationAccess) noexcept
{
    m_Dependency.srcAccessMask = p_SourceAccess;
    m_Dependency.dstAccessMask = p_DestinationAccess;
    return *this;
}
RenderPass::DependencyBuilder &RenderPass::DependencyBuilder::SetFlags(const VkDependencyFlags p_Flags) noexcept
{
    m_Dependency.dependencyFlags = p_Flags;
    return *this;
}
RenderPass::Builder &RenderPass::DependencyBuilder::EndDependency() noexcept
{
    return *m_Builder;
}

} // namespace VKit