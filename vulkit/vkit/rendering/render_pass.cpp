#include "vkit/core/pch.hpp"
#include "vkit/rendering/render_pass.hpp"

namespace VKit
{

Result<RenderPass> RenderPass::Builder::Build() const
{
    const LogicalDevice::Proxy proxy = m_Device->CreateProxy();
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkCreateRenderPass, Result<RenderPass>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkDestroyRenderPass, Result<RenderPass>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkDestroyFramebuffer, Result<RenderPass>);

    if (m_Subpasses.IsEmpty())
        return Result<RenderPass>::Error(VK_ERROR_INITIALIZATION_FAILED, "Render must have at least one subpass");

    TKit::StaticArray16<Attachment> attachments;
    TKit::StaticArray16<VkAttachmentDescription> attDescriptions;
    for (const AttachmentBuilder &attachment : m_Attachments)
    {
        TKit::StaticArray16<VkFormat> formats = attachment.m_Formats;
        if (formats.IsEmpty())
        {
            if (attachment.m_Attachment.Flags & AttachmentFlag_Color)
                formats.Append(VK_FORMAT_B8G8R8A8_SRGB);
            else if ((attachment.m_Attachment.Flags & AttachmentFlag_Depth) &&
                     (attachment.m_Attachment.Flags & AttachmentFlag_Stencil))
                formats.Append(VK_FORMAT_D32_SFLOAT_S8_UINT);
            else if (attachment.m_Attachment.Flags & AttachmentFlag_Depth)
                formats.Append(VK_FORMAT_D32_SFLOAT);
            else if (attachment.m_Attachment.Flags & AttachmentFlag_Stencil)
                formats.Append(VK_FORMAT_S8_UINT);
        }
        VkFormatFeatureFlags flags = 0;
        if (attachment.m_Attachment.Flags & AttachmentFlag_Color)
            flags = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
        else if ((attachment.m_Attachment.Flags & AttachmentFlag_Depth) ||
                 (attachment.m_Attachment.Flags & AttachmentFlag_Stencil))
            flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

        const auto result = m_Device->FindSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL, flags);
        if (!result)
            return Result<RenderPass>::Error(result.GetError());

        Attachment att = attachment.m_Attachment;
        att.Description.format = result.GetValue();

        attachments.Append(att);
        attDescriptions.Append(att.Description);
    }

    TKit::StaticArray8<VkSubpassDescription> subpasses;
    for (const SubpassBuilder &subpass : m_Subpasses)
        subpasses.Append(subpass.m_Description);

    TKit::StaticArray8<VkSubpassDependency> dependencies;
    for (const DependencyBuilder &dependency : m_Dependencies)
        dependencies.Append(dependency.m_Dependency);

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = attDescriptions.GetSize();
    createInfo.pAttachments = attDescriptions.GetData();
    createInfo.subpassCount = subpasses.GetSize();
    createInfo.pSubpasses = subpasses.GetData();
    createInfo.dependencyCount = dependencies.GetSize();
    createInfo.pDependencies = dependencies.GetData();
    createInfo.flags = m_Flags;

    VkRenderPass renderPass;
    const VkResult result = proxy.Table->CreateRenderPass(proxy, &createInfo, proxy.AllocationCallbacks, &renderPass);
    if (result != VK_SUCCESS)
        return Result<RenderPass>::Error(result, "Failed to create render pass");

    RenderPass::Info info{};
    info.Allocator = m_Allocator;
    info.Attachments = attachments;
    info.ImageCount = m_ImageCount;

    return Result<RenderPass>::Ok(proxy, renderPass, info);
}

void RenderPass::destroy() const
{
    TKIT_ASSERT(m_RenderPass, "[VULKIT] Render pass is already destroyed");
    m_Device.Table->DestroyRenderPass(m_Device, m_RenderPass, m_Device.AllocationCallbacks);
}

void RenderPass::Destroy()
{
    destroy();
    m_RenderPass = VK_NULL_HANDLE;
}
void RenderPass::SubmitForDeletion(DeletionQueue &p_Queue) const
{
    const RenderPass renderPass = *this;
    p_Queue.Push([renderPass] { renderPass.destroy(); });
}

void RenderPass::Resources::destroy() const
{
    for (const Image &image : m_Images)
        m_ImageFactory.DestroyImage(image);

    const LogicalDevice::Proxy &device = m_ImageFactory.GetDevice();
    for (const VkFramebuffer &frameBuffer : m_FrameBuffers)
        device.Table->DestroyFramebuffer(device, frameBuffer, device.AllocationCallbacks);
}
void RenderPass::Resources::Destroy()
{
    destroy();
    m_Images.Clear();
    m_FrameBuffers.Clear();
}
void RenderPass::Resources::SubmitForDeletion(DeletionQueue &p_Queue) const
{
    const Resources resources = *this;
    p_Queue.Push([resources] { resources.destroy(); });
}

RenderPass::AttachmentBuilder &RenderPass::Builder::BeginAttachment(const AttachmentFlags p_Flags)
{
    return m_Attachments.Append(this, p_Flags);
}
RenderPass::SubpassBuilder &RenderPass::Builder::BeginSubpass(const VkPipelineBindPoint p_BindPoint)
{
    return m_Subpasses.Append(this, p_BindPoint);
}
RenderPass::DependencyBuilder &RenderPass::Builder::BeginDependency(const u32 p_SourceSubpass,
                                                                    const u32 p_DestinationSubpass)
{
    return m_Dependencies.Append(this, p_SourceSubpass, p_DestinationSubpass);
}
RenderPass::Builder &RenderPass::Builder::SetAllocator(const VmaAllocator p_Allocator)
{
    m_Allocator = p_Allocator;
    return *this;
}
RenderPass::Builder &RenderPass::Builder::SetFlags(const VkRenderPassCreateFlags p_Flags)
{
    m_Flags = p_Flags;
    return *this;
}
RenderPass::Builder &RenderPass::Builder::AddFlags(const VkRenderPassCreateFlags p_Flags)
{
    m_Flags |= p_Flags;
    return *this;
}
RenderPass::Builder &RenderPass::Builder::RemoveFlags(const VkRenderPassCreateFlags p_Flags)
{
    m_Flags &= ~p_Flags;
    return *this;
}

RenderPass::AttachmentBuilder::AttachmentBuilder(RenderPass::Builder *p_Builder, const AttachmentFlags p_Flags)
    : m_Builder(p_Builder)
{
    TKIT_ASSERT(p_Flags, "[VULKIT] Attachment must have at least one type flag");
    TKIT_ASSERT(!((p_Flags & AttachmentFlag_Color) && (p_Flags & AttachmentFlag_Depth)),
                "[VULKIT] Attachment must be color or depth, not both");
    TKIT_ASSERT(!((p_Flags & AttachmentFlag_Color) && (p_Flags & AttachmentFlag_Stencil)),
                "[VULKIT] Attachment must be color or stencil, not both");

    m_Attachment.Description.samples = VK_SAMPLE_COUNT_1_BIT;
    m_Attachment.Description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_Attachment.Description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_Attachment.Description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_Attachment.Description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_Attachment.Description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_Attachment.Description.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_Attachment.Description.flags = 0;
    m_Attachment.Flags = p_Flags;

    if (p_Flags & AttachmentFlag_Color)
    {
        m_Attachment.Description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_Attachment.Description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }
    if (p_Flags & AttachmentFlag_Depth)
    {
        m_Attachment.Description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_Attachment.Description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    if (p_Flags & AttachmentFlag_Stencil)
    {
        m_Attachment.Description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_Attachment.Description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
}

RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetLoadOperation(
    const VkAttachmentLoadOp p_Operation, const VkAttachmentLoadOp p_StencilOperation)
{
    m_Attachment.Description.loadOp = p_Operation;
    if (p_StencilOperation != VK_ATTACHMENT_LOAD_OP_MAX_ENUM)
        m_Attachment.Description.stencilLoadOp = p_StencilOperation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetStoreOperation(
    const VkAttachmentStoreOp p_Operation, const VkAttachmentStoreOp p_StencilOperation)
{
    m_Attachment.Description.storeOp = p_Operation;
    if (p_StencilOperation != VK_ATTACHMENT_STORE_OP_MAX_ENUM)
        m_Attachment.Description.stencilStoreOp = p_StencilOperation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetStencilLoadOperation(
    const VkAttachmentLoadOp p_Operation)
{
    m_Attachment.Description.stencilLoadOp = p_Operation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetStencilStoreOperation(
    const VkAttachmentStoreOp p_Operation)
{
    m_Attachment.Description.stencilStoreOp = p_Operation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::RequestFormat(const VkFormat p_Format)
{
    m_Formats.Insert(m_Formats.begin(), p_Format);
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::AllowFormat(const VkFormat p_Format)
{
    m_Formats.Append(p_Format);
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetLayouts(const VkImageLayout p_InitialLayout,
                                                                         const VkImageLayout p_FinalLayout)
{
    m_Attachment.Description.initialLayout = p_InitialLayout;
    m_Attachment.Description.finalLayout = p_FinalLayout;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetInitialLayout(const VkImageLayout p_Layout)
{
    m_Attachment.Description.initialLayout = p_Layout;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetFinalLayout(const VkImageLayout p_Layout)
{
    m_Attachment.Description.finalLayout = p_Layout;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetSampleCount(const VkSampleCountFlagBits p_Samples)
{
    m_Attachment.Description.samples = p_Samples;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetFlags(const VkAttachmentDescriptionFlags p_Flags)
{
    m_Attachment.Description.flags = p_Flags;
    return *this;
}
RenderPass::Builder &RenderPass::AttachmentBuilder::EndAttachment()
{
    return *m_Builder;
}

RenderPass::SubpassBuilder::SubpassBuilder(RenderPass::Builder *p_Builder, const VkPipelineBindPoint p_BindPoint)
    : m_Builder(p_Builder)
{
    m_Description.pipelineBindPoint = p_BindPoint;
    m_DepthStencilAttachment.attachment = TKit::Limits<u32>::Max();
}

RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddColorAttachment(const u32 p_AttachmentIndex,
                                                                           const VkImageLayout p_Layout,
                                                                           const u32 p_ResolveIndex)
{
    m_ColorAttachments.Append(VkAttachmentReference{p_AttachmentIndex, p_Layout});
    if (p_ResolveIndex != TKit::Limits<u32>::Max())
    {
        m_ResolveAttachments.Append(VkAttachmentReference{p_ResolveIndex, p_Layout});
        TKIT_ASSERT(m_ResolveAttachments.GetSize() == m_ColorAttachments.GetSize(),
                    "[VULKIT] Mismatched color and resolve attachments");
    }
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddColorAttachment(const u32 p_AttachmentIndex,
                                                                           const u32 p_ResolveIndex)
{
    return AddColorAttachment(p_AttachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, p_ResolveIndex);
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddInputAttachment(const u32 p_AttachmentIndex,
                                                                           const VkImageLayout p_Layout)
{
    m_InputAttachments.Append(VkAttachmentReference{p_AttachmentIndex, p_Layout});
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddPreserveAttachment(const u32 p_AttachmentIndex)
{
    m_PreserveAttachments.Append(p_AttachmentIndex);
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::SetDepthStencilAttachment(const u32 p_AttachmentIndex,
                                                                                  const VkImageLayout p_Layout)
{
    m_DepthStencilAttachment = {p_AttachmentIndex, p_Layout};
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::SetFlags(const VkSubpassDescriptionFlags p_Flags)
{
    m_Description.flags = p_Flags;
    return *this;
}
RenderPass::Builder &RenderPass::SubpassBuilder::EndSubpass()
{
    m_Description.colorAttachmentCount = m_ColorAttachments.GetSize();
    m_Description.pColorAttachments = m_ColorAttachments.IsEmpty() ? nullptr : m_ColorAttachments.GetData();
    m_Description.inputAttachmentCount = m_InputAttachments.GetSize();
    m_Description.pInputAttachments = m_InputAttachments.IsEmpty() ? nullptr : m_InputAttachments.GetData();
    m_Description.preserveAttachmentCount = m_PreserveAttachments.GetSize();
    m_Description.pPreserveAttachments = m_PreserveAttachments.IsEmpty() ? nullptr : m_PreserveAttachments.GetData();
    m_Description.pResolveAttachments = m_ResolveAttachments.IsEmpty() ? nullptr : m_ResolveAttachments.GetData();
    m_Description.pDepthStencilAttachment =
        m_DepthStencilAttachment.attachment == TKit::Limits<u32>::Max() ? nullptr : &m_DepthStencilAttachment;

    return *m_Builder;
}

RenderPass::DependencyBuilder::DependencyBuilder(RenderPass::Builder *p_Builder, const u32 p_SourceSubpass,
                                                 const u32 p_DestinationSubpass)
    : m_Builder(p_Builder)
{
    m_Dependency.srcSubpass = p_SourceSubpass;
    m_Dependency.dstSubpass = p_DestinationSubpass;
}

RenderPass::DependencyBuilder &RenderPass::DependencyBuilder::SetStageMask(
    const VkPipelineStageFlags p_SourceStage, const VkPipelineStageFlags p_DestinationStage)
{
    m_Dependency.srcStageMask = p_SourceStage;
    m_Dependency.dstStageMask = p_DestinationStage;
    return *this;
}
RenderPass::DependencyBuilder &RenderPass::DependencyBuilder::SetAccessMask(const VkAccessFlags p_SourceAccess,
                                                                            const VkAccessFlags p_DestinationAccess)
{
    m_Dependency.srcAccessMask = p_SourceAccess;
    m_Dependency.dstAccessMask = p_DestinationAccess;
    return *this;
}
RenderPass::DependencyBuilder &RenderPass::DependencyBuilder::SetFlags(const VkDependencyFlags p_Flags)
{
    m_Dependency.dependencyFlags = p_Flags;
    return *this;
}
RenderPass::Builder &RenderPass::DependencyBuilder::EndDependency()
{
    return *m_Builder;
}

} // namespace VKit
