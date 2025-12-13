#include "vkit/core/pch.hpp"
#include "vkit/resource/image.hpp"
#include "vkit/resource/buffer.hpp"
#include "vkit/rendering/command_pool.hpp"

namespace VKit
{
static VkImageViewType getImageViewType(const VkImageType p_Type)
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
VkImageAspectFlags Detail::DeduceAspectMask(const Image::Flags p_Flags)
{
    if (p_Flags & Image::Flag_ColorAttachment)
        return VK_IMAGE_ASPECT_COLOR_BIT;
    else if ((p_Flags & Image::Flag_DepthAttachment) && (p_Flags & Image::Flag_StencilAttachment))
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    else if (p_Flags & Image::Flag_DepthAttachment)
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (p_Flags & Image::Flag_StencilAttachment)
        return VK_IMAGE_ASPECT_STENCIL_BIT;

    TKIT_LOG_WARNING("[VULKIT] Unable to deduce aspect mask. Using 'VK_IMAGE_ASPECT_NONE'");
    return VK_IMAGE_ASPECT_NONE;
}
static VkImageSubresourceRange createRange(const VkImageCreateInfo &p_Info, const Image::Flags p_Flags)
{
    VkImageSubresourceRange range{};
    range.aspectMask |= Detail::DeduceAspectMask(p_Flags);

    range.baseMipLevel = 0;
    range.levelCount = p_Info.mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = p_Info.arrayLayers;
    return range;
}
Image::Builder::Builder(const LogicalDevice::Proxy &p_Device, const VmaAllocator p_Allocator,
                        const VkExtent2D &p_Extent, const VkFormat p_Format, const Flags p_Flags)
    : Image::Builder(p_Device, p_Allocator, VkExtent3D{.width = p_Extent.width, .height = p_Extent.height, .depth = 1},
                     p_Format, p_Flags)
{
}

static VkImageViewCreateInfo createDefaultImageViewInfo(const VkImage p_Image, const VkImageViewType p_Type,
                                                        const VkImageSubresourceRange &p_Range)
{
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = p_Image;
    info.viewType = p_Type;
    info.format = VK_FORMAT_UNDEFINED;
    info.subresourceRange = p_Range;
    return info;
}

Image::Builder::Builder(const LogicalDevice::Proxy &p_Device, const VmaAllocator p_Allocator,
                        const VkExtent3D &p_Extent, const VkFormat p_Format, const Flags p_Flags)
    : m_Device(p_Device), m_Allocator(p_Allocator), m_Flags(p_Flags)
{
    m_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    m_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    m_ImageInfo.extent = p_Extent;
    m_ImageInfo.mipLevels = 1;
    m_ImageInfo.arrayLayers = 1;
    m_ImageInfo.format = p_Format;
    m_ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    m_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    m_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    m_ImageInfo.flags = 0;
    if (p_Flags & Flag_ColorAttachment)
        m_ImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else if ((p_Flags & Flag_DepthAttachment) || (p_Flags & Flag_StencilAttachment))
        m_ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    if (p_Flags & Flag_InputAttachment)
        m_ImageInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    if (p_Flags & Flag_Sampled)
        m_ImageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

    m_ViewInfo = createDefaultImageViewInfo(VK_NULL_HANDLE, getImageViewType(m_ImageInfo.imageType),
                                            createRange(m_ImageInfo, p_Flags));
}

Image::Info Image::FromSwapChain(const VkFormat p_Format, const VkExtent2D &p_Extent, const Flags p_Flags)
{
    Info info;
    info.Allocation = VK_NULL_HANDLE;
    info.Allocator = VK_NULL_HANDLE;
    info.Width = p_Extent.width;
    info.Height = p_Extent.height;
    info.Depth = 1;
    info.Format = p_Format;
    info.Flags = p_Flags;
    return info;
}

Result<Image> Image::Builder::Build() const
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkCreateImageView, Result<Image>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(m_Device.Table, vkDestroyImageView, Result<Image>);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkImage image;
    VmaAllocation allocation;

    const VkResult result = vmaCreateImage(m_Allocator, &m_ImageInfo, &allocInfo, &image, &allocation, nullptr);
    if (result != VK_SUCCESS)
        return Result<Image>::Error(result, "Failed to create image");

    Info info;
    info.Allocator = m_Allocator;
    info.Allocation = allocation;
    info.Format = m_ImageInfo.format;
    info.Width = m_ImageInfo.extent.width;
    info.Height = m_ImageInfo.extent.height;
    info.Depth = m_ImageInfo.extent.depth;
    info.Flags = m_Flags;

    Image img{m_Device, image, m_ImageInfo.initialLayout, info};
    if (m_ViewInfo.format == VK_FORMAT_UNDEFINED)
        return Result<Image>::Ok(img);

    VkImageViewCreateInfo vinfo = m_ViewInfo;
    vinfo.image = image;
    const auto vres = img.CreateImageView(vinfo);
    if (!vres)
        return Result<Image>::Error(vres.GetError());

    return Result<Image>::Ok(img);
}

Result<VkImageView> Image::CreateImageView(const VkImageViewCreateInfo &p_Info)
{
    const VkResult result =
        m_Device.Table->CreateImageView(m_Device, &p_Info, m_Device.AllocationCallbacks, &m_ImageView);
    if (result != VK_SUCCESS)
        return Result<VkImageView>::Error(result, "Failed to create image view");
    return Result<VkImageView>::Ok(m_ImageView);
}

void Image::TransitionLayout(const VkCommandBuffer p_CommandBuffer, const VkImageLayout p_Layout,
                             const TransitionInfo &p_Info)
{
    if (m_Layout == p_Layout)
        return;
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_Layout;
    barrier.newLayout = p_Layout;
    barrier.srcQueueFamilyIndex = p_Info.SrcFamilyIndex;
    barrier.dstQueueFamilyIndex = p_Info.DstFamilyIndex;
    barrier.image = m_Image;
    barrier.subresourceRange = p_Info.Range;
    barrier.srcAccessMask = p_Info.SrcAccess;
    barrier.dstAccessMask = p_Info.DstAccess;
    if (p_Info.Range.aspectMask == VK_IMAGE_ASPECT_NONE)
        barrier.subresourceRange.aspectMask = Detail::DeduceAspectMask(m_Info.Flags);

    m_Device.Table->CmdPipelineBarrier(p_CommandBuffer, p_Info.SrcStage, p_Info.DstStage, 0, 0, nullptr, 0, nullptr, 1,
                                       &barrier);
    m_Layout = p_Layout;
}

void Image::CopyFromImage(const VkCommandBuffer p_CommandBuffer, const Image &p_Source, const ImageCopy &p_Info)
{
    const VkOffset3D &soff = p_Info.SrcOffset;
    const VkOffset3D &doff = p_Info.DstOffset;
    const VkExtent3D &ext = p_Info.Extent;
    const Image::Info &info = p_Source.GetInfo();

    VkImageCopy copy;
    copy.srcOffset = soff;
    copy.dstOffset = doff;

    VkExtent3D &cext = copy.extent;
    cext.width =
        ext.width == TKit::Limits<u32>::Max() ? std::min(info.Width - soff.x, m_Info.Width - doff.x) : ext.width;
    cext.height =
        ext.height == TKit::Limits<u32>::Max() ? std::min(info.Height - soff.y, m_Info.Height - doff.y) : ext.height;
    cext.depth =
        ext.depth == TKit::Limits<u32>::Max() ? std::min(info.Depth - soff.z, m_Info.Depth - doff.z) : ext.depth;

    // i know this is so futile, validation layers would already catch this but well...
    TKIT_ASSERT(cext.width <= info.Width - soff.x, "[ONYX] Specified width exceeds source image width");
    TKIT_ASSERT(cext.height <= info.Height - soff.y, "[ONYX] Specified height exceeds source image height");
    TKIT_ASSERT(cext.depth <= info.Depth - soff.z, "[ONYX] Specified depth exceeds source image depth");

    TKIT_ASSERT(cext.width <= m_Info.Width - doff.x, "[ONYX] Specified width exceeds destination image width");
    TKIT_ASSERT(cext.height <= m_Info.Height - doff.y, "[ONYX] Specified height exceeds destination image height");
    TKIT_ASSERT(cext.depth <= m_Info.Depth - doff.z, "[ONYX] Specified depth exceeds destination image depth");

    m_Device.Table->CmdCopyImage(p_CommandBuffer, p_Source, p_Source.GetLayout(), m_Image, m_Layout, 1, &copy);
}

void Image::CopyFromBuffer(const VkCommandBuffer p_CommandBuffer, const Buffer &p_Source, const BufferImageCopy &p_Info)
{
    const VkOffset3D &off = p_Info.ImageOffset;
    const VkExtent3D &ext = p_Info.Extent;

    VkBufferImageCopy copy;
    copy.bufferImageHeight = p_Info.BufferImageHeight;
    copy.bufferOffset = p_Info.BufferOffset;
    copy.imageOffset = off;
    copy.bufferRowLength = p_Info.BufferRowLength;
    copy.imageSubresource = p_Info.Subresource;
    if (copy.imageSubresource.aspectMask == VK_IMAGE_ASPECT_NONE)
        copy.imageSubresource.aspectMask = Detail::DeduceAspectMask(m_Info.Flags);

    VkExtent3D &cext = copy.imageExtent;
    cext.width = ext.width == TKit::Limits<u32>::Max() ? (m_Info.Width - off.x) : ext.width;
    cext.height = ext.height == TKit::Limits<u32>::Max() ? (m_Info.Height - off.y) : ext.height;
    cext.depth = ext.depth == TKit::Limits<u32>::Max() ? (m_Info.Depth - off.z) : ext.depth;

    // i know this is so futile, validation layers would already catch this but well...
    TKIT_ASSERT(cext.width <= m_Info.Width - off.x, "[ONYX] Specified width exceeds destination image width");
    TKIT_ASSERT(cext.height <= m_Info.Height - off.y, "[ONYX] Specified height exceeds destination image height");
    TKIT_ASSERT(cext.depth <= m_Info.Depth - off.z, "[ONYX] Specified depth exceeds destination image depth");
    TKIT_ASSERT(p_Source.GetInfo().Size - p_Info.BufferOffset >=
                    GetSize(p_Info.BufferRowLength, p_Info.BufferImageHeight),
                "[ONYX] Buffer is not large enough to fit image");

    m_Device.Table->CmdCopyBufferToImage(p_CommandBuffer, p_Source, m_Image, m_Layout, 1, &copy);
}

Result<> Image::CopyFromImage(CommandPool &p_Pool, VkQueue p_Queue, const Image &p_Source, const ImageCopy &p_Info)
{
    const auto cres = p_Pool.BeginSingleTimeCommands();
    if (!cres)
        return Result<>::Error(cres.GetError());

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromImage(cmd, p_Source, p_Info);
    return p_Pool.EndSingleTimeCommands(cmd, p_Queue);
}

Result<> Image::CopyFromBuffer(CommandPool &p_Pool, VkQueue p_Queue, const Buffer &p_Source,
                               const BufferImageCopy &p_Info)
{
    const auto cres = p_Pool.BeginSingleTimeCommands();
    if (!cres)
        return Result<>::Error(cres.GetError());

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromBuffer(cmd, p_Source, p_Info);
    return p_Pool.EndSingleTimeCommands(cmd, p_Queue);
}

Result<> Image::UploadFromHost(CommandPool &p_Pool, const VkQueue p_Queue, const HostData &p_Data,
                               VkImageLayout p_FinalLayout)
{
    if (p_FinalLayout == VK_IMAGE_LAYOUT_UNDEFINED)
        p_FinalLayout = m_Layout;

    const VkDeviceSize size = GetSize();
    TKIT_ASSERT(p_Data.Channels == GetBytesPerPixel(),
                "[VULKIT] The number of channels must match the bytes per pixel of the image");
    TKIT_ASSERT(size == p_Data.Width * p_Data.Height * p_Data.Depth * p_Data.Channels,
                "[VULKIT] When uploading host-side image, both images must match in size");

    auto bres = Buffer::Builder(m_Device, m_Info.Allocator, Buffer::Flag_HostMapped | Buffer::Flag_StagingBuffer)
                    .SetSize(size)
                    .Build();
    if (!bres)
        return Result<>::Error(bres.GetError());

    Buffer &staging = bres.GetValue();
    staging.Write(p_Data.Data);
    staging.Flush();

    const auto cres = p_Pool.BeginSingleTimeCommands();
    if (!cres)
        return Result<>::Error(cres.GetError());

    const VkCommandBuffer cmd = cres.GetValue();

    TransitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     {.SrcAccess = 0,
                      .DstAccess = VK_ACCESS_TRANSFER_WRITE_BIT,
                      .SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                      .DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT});

    CopyFromBuffer(cmd, staging);

    TransitionLayout(cmd, p_FinalLayout,
                     {.SrcAccess = VK_ACCESS_TRANSFER_WRITE_BIT,
                      .DstAccess = 0,
                      .SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
                      .DstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT});

    const auto endres = p_Pool.EndSingleTimeCommands(cmd, p_Queue);

    staging.Destroy();
    return endres;
}

VkDeviceSize Image::GetBytesPerPixel(const VkFormat p_Format)
{
    switch (p_Format)
    {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8_SRGB:
        return 1;

    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8_SRGB:
        return 2;

    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_B8G8R8_UNORM:
    case VK_FORMAT_B8G8R8_SNORM:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8_SRGB:
        return 3;

    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_B8G8R8A8_SRGB:
        return 4;

    case VK_FORMAT_R16G16B16_UNORM:
    case VK_FORMAT_R16G16B16_SNORM:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16_SFLOAT:
        return 6;

    case VK_FORMAT_R64_UINT:
    case VK_FORMAT_R64_SINT:
    case VK_FORMAT_R64_SFLOAT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 8;

    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32_SFLOAT:
        return 12;

    case VK_FORMAT_R64G64_UINT:
    case VK_FORMAT_R64G64_SINT:
    case VK_FORMAT_R64G64_SFLOAT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 16;

    case VK_FORMAT_D16_UNORM:
        return 2;

    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return 4;

    case VK_FORMAT_D32_SFLOAT:
        return 4;

    case VK_FORMAT_S8_UINT:
        return 1;

    case VK_FORMAT_D24_UNORM_S8_UINT:
        return 4;

    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return 5;
    default:
        TKIT_LOG_WARNING("[VULKIT] Unrecognized vulkan format when resolving the number of bytes per pixel for it");
        return 0;
    }
    TKIT_LOG_WARNING("[VULKIT] Unrecognized vulkan format when resolving the number of bytes per pixel for it");
    return 0;
}

VkDeviceSize Image::GetSize(const u32 p_BufferRowLength, const u32 p_BufferImageHeight) const
{
    const VkDeviceSize ppixel = GetBytesPerPixel();
    const u32 rowTexels = p_BufferRowLength == 0 ? m_Info.Width : p_BufferRowLength;
    const u32 imgTexels = p_BufferImageHeight == 0 ? m_Info.Height : p_BufferImageHeight;

    const VkDeviceSize rowStride = static_cast<VkDeviceSize>(rowTexels) * ppixel;
    const VkDeviceSize sliceStride = static_cast<VkDeviceSize>(imgTexels) * rowStride;

    return m_Info.Width * ppixel + (m_Info.Height - 1) * rowStride + (m_Info.Depth - 1) * sliceStride;
}

void Image::Destroy()
{
    DestroyImageView();
    if (m_Image && m_Info.Allocation)
        vmaDestroyImage(m_Info.Allocator, m_Image, m_Info.Allocation);
    m_Info = {};
    m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
}
void Image::DestroyImageView()
{
    if (m_ImageView)
        m_Device.Table->DestroyImageView(m_Device, m_ImageView, m_Device.AllocationCallbacks);
    m_ImageView = VK_NULL_HANDLE;
}
Image::Builder &Image::Builder::SetImageType(const VkImageType p_Type)
{
    m_ImageInfo.imageType = p_Type;
    return *this;
}

Image::Builder &Image::Builder::SetDepth(const u32 p_Depth)
{
    m_ImageInfo.extent.depth = p_Depth;
    return *this;
}

Image::Builder &Image::Builder::SetMipLevels(const u32 p_Levels)
{
    m_ImageInfo.mipLevels = p_Levels;
    m_ViewInfo.subresourceRange.levelCount = p_Levels;
    return *this;
}

Image::Builder &Image::Builder::SetArrayLayers(const u32 p_Layers)
{
    m_ImageInfo.arrayLayers = p_Layers;
    m_ViewInfo.subresourceRange.layerCount = p_Layers;
    return *this;
}

Image::Builder &Image::Builder::SetTiling(const VkImageTiling p_Tiling)
{
    m_ImageInfo.tiling = p_Tiling;
    return *this;
}

Image::Builder &Image::Builder::SetInitialLayout(const VkImageLayout p_Layout)
{
    m_ImageInfo.initialLayout = p_Layout;
    return *this;
}

Image::Builder &Image::Builder::SetSamples(const VkSampleCountFlagBits p_Samples)
{
    m_ImageInfo.samples = p_Samples;
    return *this;
}

Image::Builder &Image::Builder::SetSharingMode(const VkSharingMode p_Mode)
{
    m_ImageInfo.sharingMode = p_Mode;
    return *this;
}

Image::Builder &Image::Builder::SetFlags(const VkImageCreateFlags p_Flags)
{
    m_ImageInfo.flags = p_Flags;
    return *this;
}

Image::Builder &Image::Builder::SetUsage(const VkImageUsageFlags p_Flags)
{
    m_ImageInfo.usage = p_Flags;
    return *this;
}
Image::Builder &Image::Builder::SetImageCreateInfo(const VkImageCreateInfo &p_Info)
{
    m_ImageInfo = p_Info;
    return *this;
}

Image::Builder &Image::Builder::WithImageView()
{
    m_ViewInfo.format = m_ImageInfo.format;
    return *this;
}
Image::Builder &Image::Builder::WithImageView(const VkImageViewCreateInfo &p_Info)
{
    TKIT_ASSERT(!p_Info.image, "[ONYX] The image must be set to null when passing a image view create info because it "
                               "will be replaced with the newly created image");
    m_ViewInfo = p_Info;
    return *this;
}
Image::Builder &Image::Builder::WithImageView(const VkImageSubresourceRange &p_Range)
{
    m_ViewInfo.format = m_ImageInfo.format;
    m_ViewInfo.subresourceRange = p_Range;
    return *this;
}

} // namespace VKit
