#include "vkit/core/pch.hpp"
#include "vkit/state/render_pass.hpp"
#include "tkit/container/stack_array.hpp"

namespace VKit
{
Result<RenderPass> RenderPass::Builder::Build() const
{
    const ProxyDevice proxy = m_Device->CreateProxy();
    TKIT_ASSERT(!m_Subpasses.IsEmpty(), "[VULKIT][RENDER-PASS] Render pass must have at least one subpass");

    TKit::StackArray<Attachment> attachments;
    attachments.Reserve(m_Attachments.GetSize());

    TKit::StackArray<VkAttachmentDescription> attDescriptions;
    attDescriptions.Reserve(m_Attachments.GetSize());

    for (const AttachmentBuilder &attachment : m_Attachments)
    {
        TKit::StackArray<VkFormat> formats;
        formats.Reserve(attachment.m_Formats.GetCapacity() + 4);
        formats = attachment.m_Formats;
        if (formats.IsEmpty())
        {
            if (attachment.m_Attachment.Flags & DeviceImageFlag_ColorAttachment)
                formats.Append(VK_FORMAT_B8G8R8A8_SRGB);
            else if ((attachment.m_Attachment.Flags & DeviceImageFlag_DepthAttachment) &&
                     (attachment.m_Attachment.Flags & DeviceImageFlag_StencilAttachment))
                formats.Append(VK_FORMAT_D32_SFLOAT_S8_UINT);
            else if (attachment.m_Attachment.Flags & DeviceImageFlag_DepthAttachment)
                formats.Append(VK_FORMAT_D32_SFLOAT);
            else if (attachment.m_Attachment.Flags & DeviceImageFlag_StencilAttachment)
                formats.Append(VK_FORMAT_S8_UINT);
        }
        VkFormatFeatureFlags flags = 0;
        if (attachment.m_Attachment.Flags & DeviceImageFlag_ColorAttachment)
            flags = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
        else if ((attachment.m_Attachment.Flags & DeviceImageFlag_DepthAttachment) ||
                 (attachment.m_Attachment.Flags & DeviceImageFlag_StencilAttachment))
            flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

        const auto result = m_Device->FindSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL, flags);
        TKIT_RETURN_ON_ERROR(result);

        Attachment att = attachment.m_Attachment;
        att.Description.format = result.GetValue();

        attachments.Append(att);
        attDescriptions.Append(att.Description);
    }

    TKit::StackArray<VkSubpassDescription> subpasses;
    subpasses.Reserve(m_Subpasses.GetSize());
    for (const SubpassBuilder &subpass : m_Subpasses)
        subpasses.Append(subpass.m_Description);

    TKit::StackArray<VkSubpassDependency> dependencies;
    dependencies.Reserve(m_Dependencies.GetSize());
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
        return Result<RenderPass>::Error(result);

    RenderPass::Info info{};
    info.Allocator = m_Allocator;
    info.Attachments = attachments;
    info.ImageCount = m_ImageCount;

    return Result<RenderPass>::Ok(proxy, renderPass, info);
}

void RenderPass::Destroy()
{
    if (m_RenderPass)
    {
        m_Device.Table->DestroyRenderPass(m_Device, m_RenderPass, m_Device.AllocationCallbacks);
        m_RenderPass = VK_NULL_HANDLE;
    }
}
void RenderPass::Resources::Destroy()
{
    for (DeviceImage image : m_Images)
        image.Destroy();

    for (const VkFramebuffer &frameBuffer : m_FrameBuffers)
        m_Device.Table->DestroyFramebuffer(m_Device, frameBuffer, m_Device.AllocationCallbacks);

    m_Images.Clear();
    m_FrameBuffers.Clear();
}
RenderPass::AttachmentBuilder &RenderPass::Builder::BeginAttachment(const DeviceImageFlags flags)
{
    return m_Attachments.Append(this, flags);
}
RenderPass::SubpassBuilder &RenderPass::Builder::BeginSubpass(const VkPipelineBindPoint bindPoint)
{
    return m_Subpasses.Append(this, bindPoint);
}
RenderPass::DependencyBuilder &RenderPass::Builder::BeginDependency(const u32 sourceSubpass,
                                                                    const u32 destinationSubpass)
{
    return m_Dependencies.Append(this, sourceSubpass, destinationSubpass);
}
RenderPass::Builder &RenderPass::Builder::SetAllocator(const VmaAllocator allocator)
{
    m_Allocator = allocator;
    return *this;
}
RenderPass::Builder &RenderPass::Builder::SetFlags(const VkRenderPassCreateFlags flags)
{
    m_Flags = flags;
    return *this;
}
RenderPass::Builder &RenderPass::Builder::AddFlags(const VkRenderPassCreateFlags flags)
{
    m_Flags |= flags;
    return *this;
}
RenderPass::Builder &RenderPass::Builder::RemoveFlags(const VkRenderPassCreateFlags flags)
{
    m_Flags &= ~flags;
    return *this;
}

RenderPass::AttachmentBuilder::AttachmentBuilder(RenderPass::Builder *builder, const DeviceImageFlags flags)
    : m_Builder(builder)
{
    TKIT_ASSERT(flags, "[VULKIT][RENDER-PASS] Attachment must have at least one type flag");
    TKIT_ASSERT(!((flags & DeviceImageFlag_ColorAttachment) && (flags & DeviceImageFlag_DepthAttachment)),
                "[VULKIT][RENDER-PASS] Attachment must be color or depth, not both");
    TKIT_ASSERT(!((flags & DeviceImageFlag_ColorAttachment) && (flags & DeviceImageFlag_StencilAttachment)),
                "[VULKIT][RENDER-PASS] Attachment must be color or stencil, not both");

    m_Attachment.Description.samples = VK_SAMPLE_COUNT_1_BIT;
    m_Attachment.Description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_Attachment.Description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_Attachment.Description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_Attachment.Description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_Attachment.Description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_Attachment.Description.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_Attachment.Description.flags = 0;
    m_Attachment.Flags = flags;

    if (flags & DeviceImageFlag_ColorAttachment)
    {
        m_Attachment.Description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_Attachment.Description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }
    if (flags & DeviceImageFlag_DepthAttachment)
    {
        m_Attachment.Description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_Attachment.Description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    if (flags & DeviceImageFlag_StencilAttachment)
    {
        m_Attachment.Description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_Attachment.Description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
}

RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetLoadOperation(
    const VkAttachmentLoadOp operation, const VkAttachmentLoadOp stencilOperation)
{
    m_Attachment.Description.loadOp = operation;
    if (stencilOperation != VK_ATTACHMENT_LOAD_OP_MAX_ENUM)
        m_Attachment.Description.stencilLoadOp = stencilOperation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetStoreOperation(
    const VkAttachmentStoreOp operation, const VkAttachmentStoreOp stencilOperation)
{
    m_Attachment.Description.storeOp = operation;
    if (stencilOperation != VK_ATTACHMENT_STORE_OP_MAX_ENUM)
        m_Attachment.Description.stencilStoreOp = stencilOperation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetStencilLoadOperation(
    const VkAttachmentLoadOp operation)
{
    m_Attachment.Description.stencilLoadOp = operation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetStencilStoreOperation(
    const VkAttachmentStoreOp operation)
{
    m_Attachment.Description.stencilStoreOp = operation;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::RequestFormat(const VkFormat format)
{
    m_Formats.Insert(m_Formats.begin(), format);
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::AllowFormat(const VkFormat format)
{
    m_Formats.Append(format);
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetLayouts(const VkImageLayout initialLayout,
                                                                         const VkImageLayout finalLayout)
{
    m_Attachment.Description.initialLayout = initialLayout;
    m_Attachment.Description.finalLayout = finalLayout;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetInitialLayout(const VkImageLayout layout)
{
    m_Attachment.Description.initialLayout = layout;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetFinalLayout(const VkImageLayout layout)
{
    m_Attachment.Description.finalLayout = layout;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetSampleCount(const VkSampleCountFlagBits samples)
{
    m_Attachment.Description.samples = samples;
    return *this;
}
RenderPass::AttachmentBuilder &RenderPass::AttachmentBuilder::SetFlags(const VkAttachmentDescriptionFlags flags)
{
    m_Attachment.Description.flags = flags;
    return *this;
}
RenderPass::Builder &RenderPass::AttachmentBuilder::EndAttachment()
{
    return *m_Builder;
}

RenderPass::SubpassBuilder::SubpassBuilder(RenderPass::Builder *builder, const VkPipelineBindPoint bindPoint)
    : m_Builder(builder)
{
    m_Description.pipelineBindPoint = bindPoint;
    m_DepthStencilAttachment.attachment = TKIT_U32_MAX;
}

RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddColorAttachment(const u32 attachmentIndex,
                                                                           const VkImageLayout layout,
                                                                           const u32 resolveIndex)
{
    m_ColorAttachments.Append(VkAttachmentReference{attachmentIndex, layout});
    if (resolveIndex != TKIT_U32_MAX)
    {
        m_ResolveAttachments.Append(VkAttachmentReference{resolveIndex, layout});
        TKIT_ASSERT(m_ResolveAttachments.GetSize() == m_ColorAttachments.GetSize(),
                    "[VULKIT][RENDER-PASS] Mismatched color and resolve attachments");
    }
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddColorAttachment(const u32 attachmentIndex,
                                                                           const u32 resolveIndex)
{
    return AddColorAttachment(attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, resolveIndex);
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddInputAttachment(const u32 attachmentIndex,
                                                                           const VkImageLayout layout)
{
    m_InputAttachments.Append(VkAttachmentReference{attachmentIndex, layout});
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::AddPreserveAttachment(const u32 attachmentIndex)
{
    m_PreserveAttachments.Append(attachmentIndex);
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::SetDepthStencilAttachment(const u32 attachmentIndex,
                                                                                  const VkImageLayout layout)
{
    m_DepthStencilAttachment = {attachmentIndex, layout};
    return *this;
}
RenderPass::SubpassBuilder &RenderPass::SubpassBuilder::SetFlags(const VkSubpassDescriptionFlags flags)
{
    m_Description.flags = flags;
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
        m_DepthStencilAttachment.attachment == TKIT_U32_MAX ? nullptr : &m_DepthStencilAttachment;

    return *m_Builder;
}

RenderPass::DependencyBuilder::DependencyBuilder(RenderPass::Builder *builder, const u32 sourceSubpass,
                                                 const u32 destinationSubpass)
    : m_Builder(builder)
{
    m_Dependency.srcSubpass = sourceSubpass;
    m_Dependency.dstSubpass = destinationSubpass;
}

RenderPass::DependencyBuilder &RenderPass::DependencyBuilder::SetStageMask(const VkPipelineStageFlags sourceStage,
                                                                           const VkPipelineStageFlags destinationStage)
{
    m_Dependency.srcStageMask = sourceStage;
    m_Dependency.dstStageMask = destinationStage;
    return *this;
}
RenderPass::DependencyBuilder &RenderPass::DependencyBuilder::SetAccessMask(const VkAccessFlags sourceAccess,
                                                                            const VkAccessFlags destinationAccess)
{
    m_Dependency.srcAccessMask = sourceAccess;
    m_Dependency.dstAccessMask = destinationAccess;
    return *this;
}
RenderPass::DependencyBuilder &RenderPass::DependencyBuilder::SetFlags(const VkDependencyFlags flags)
{
    m_Dependency.dependencyFlags = flags;
    return *this;
}
RenderPass::Builder &RenderPass::DependencyBuilder::EndDependency()
{
    return *m_Builder;
}

} // namespace VKit
