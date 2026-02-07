#include "vkit/core/pch.hpp"
#include "vkit/resource/image.hpp"
#include "vkit/resource/device_buffer.hpp"
#include "vkit/execution/command_pool.hpp"
#include "tkit/math/math.hpp"

namespace VKit
{
namespace Math = TKit::Math;
static VkImageViewType getImageViewType(const VkImageType type)
{
    switch (type)
    {
    case VK_IMAGE_TYPE_1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case VK_IMAGE_TYPE_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case VK_IMAGE_TYPE_3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    default:
        TKIT_LOG_WARNING("[VULKIT][DEVICE-IMAGE] Unrecognized vulkan image type");
        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }
}
VkImageAspectFlags Detail::InferAspectMask(const DeviceImageFlags flags)
{
    if (flags & DeviceImageFlag_ColorAttachment)
        return VK_IMAGE_ASPECT_COLOR_BIT;
    else if ((flags & DeviceImageFlag_DepthAttachment) && (flags & DeviceImageFlag_StencilAttachment))
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    else if (flags & DeviceImageFlag_DepthAttachment)
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (flags & DeviceImageFlag_StencilAttachment)
        return VK_IMAGE_ASPECT_STENCIL_BIT;

    TKIT_LOG_WARNING("[VULKIT][DEVICE-IMAGE] Unable to deduce aspect mask. Using 'VK_IMAGE_ASPECT_NONE'");
    return VK_IMAGE_ASPECT_NONE;
}
static VkImageSubresourceRange createRange(const VkImageCreateInfo &info, const DeviceImageFlags flags)
{
    VkImageSubresourceRange range{};
    range.aspectMask |= Detail::InferAspectMask(flags);

    range.baseMipLevel = 0;
    range.levelCount = info.mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = info.arrayLayers;
    return range;
}
DeviceImage::Builder::Builder(const ProxyDevice &device, const VmaAllocator allocator, const VkExtent2D &extent,
                              const VkFormat format, const DeviceImageFlags flags)
    : DeviceImage::Builder(device, allocator, VkExtent3D{.width = extent.width, .height = extent.height, .depth = 1},
                           format, flags)
{
}

static VkImageViewCreateInfo createDefaultImageViewInfo(const VkImage image, const VkImageViewType type,
                                                        const VkImageSubresourceRange &range)
{
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = type;
    info.format = VK_FORMAT_UNDEFINED;
    info.subresourceRange = range;
    return info;
}

DeviceImage::Builder::Builder(const ProxyDevice &device, const VmaAllocator allocator, const VkExtent3D &extent,
                              const VkFormat format, const DeviceImageFlags flags)
    : m_Device(device), m_Allocator(allocator), m_Flags(flags)
{
    m_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    m_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    m_ImageInfo.extent = extent;
    m_ImageInfo.mipLevels = 1;
    m_ImageInfo.arrayLayers = 1;
    m_ImageInfo.format = format;
    m_ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    m_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    m_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    m_ImageInfo.flags = 0;
    if (flags & DeviceImageFlag_ColorAttachment)
        m_ImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else if ((flags & DeviceImageFlag_DepthAttachment) || (flags & DeviceImageFlag_StencilAttachment))
        m_ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    if (flags & DeviceImageFlag_InputAttachment)
        m_ImageInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    if (flags & DeviceImageFlag_Sampled)
        m_ImageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (flags & DeviceImageFlag_Source)
        m_ImageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (flags & DeviceImageFlag_Destination)
        m_ImageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    m_ViewInfo = createDefaultImageViewInfo(VK_NULL_HANDLE, getImageViewType(m_ImageInfo.imageType),
                                            createRange(m_ImageInfo, flags));
}

DeviceImage::Info DeviceImage::FromSwapChain(const VkFormat format, const VkExtent2D &extent,
                                             const DeviceImageFlags flags)
{
    Info info;
    info.Allocation = VK_NULL_HANDLE;
    info.Allocator = VK_NULL_HANDLE;
    info.Width = extent.width;
    info.Height = extent.height;
    info.Depth = 1;
    info.Format = format;
    info.Flags = flags;
    return info;
}

Result<DeviceImage> DeviceImage::Builder::Build() const
{
    VmaAllocationCreateInfo allocInfo{};
    if (m_Flags & DeviceImageFlag_ForceHostVisible)
    {
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    else
    {
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    VkImage image;
    VmaAllocation allocation;

    VKIT_RETURN_IF_FAILED(vmaCreateImage(m_Allocator, &m_ImageInfo, &allocInfo, &image, &allocation, nullptr),
                          Result<DeviceImage>);

    Info info;
    info.Allocator = m_Allocator;
    info.Allocation = allocation;
    info.Format = m_ImageInfo.format;
    info.Width = m_ImageInfo.extent.width;
    info.Height = m_ImageInfo.extent.height;
    info.Depth = m_ImageInfo.extent.depth;
    info.Flags = m_Flags;

    DeviceImage img{m_Device, image, m_ImageInfo.initialLayout, info};
    if (m_ViewInfo.format == VK_FORMAT_UNDEFINED)
        return img;

    VkImageViewCreateInfo vinfo = m_ViewInfo;
    vinfo.image = image;
    const auto vres = img.CreateImageView(vinfo);
    if (!vres)
        return vres;

    return img;
}

Result<VkImageView> DeviceImage::CreateImageView(const VkImageViewCreateInfo &info)
{
    VKIT_RETURN_IF_FAILED(m_Device.Table->CreateImageView(m_Device, &info, m_Device.AllocationCallbacks, &m_ImageView),
                          Result<VkImageView>);
    return m_ImageView;
}

void DeviceImage::TransitionLayout(const VkCommandBuffer commandBuffer, const VkImageLayout layout,
                                   const TransitionInfo &info)
{
    if (m_Layout == layout)
        return;
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_Layout;
    barrier.newLayout = layout;
    barrier.srcQueueFamilyIndex = info.SrcFamilyIndex;
    barrier.dstQueueFamilyIndex = info.DstFamilyIndex;
    barrier.image = m_Image;
    barrier.subresourceRange = info.Range;
    barrier.srcAccessMask = info.SrcAccess;
    barrier.dstAccessMask = info.DstAccess;
    if (info.Range.aspectMask == VK_IMAGE_ASPECT_NONE)
        barrier.subresourceRange.aspectMask = Detail::InferAspectMask(m_Info.Flags);

    m_Device.Table->CmdPipelineBarrier(commandBuffer, info.SrcStage, info.DstStage, 0, 0, nullptr, 0, nullptr, 1,
                                       &barrier);
    m_Layout = layout;
}

void DeviceImage::CopyFromImage(const VkCommandBuffer commandBuffer, const DeviceImage &source,
                                const TKit::Span<const VkImageCopy> copy)
{
    m_Device.Table->CmdCopyImage(commandBuffer, source, source.GetLayout(), m_Image, m_Layout, copy.GetSize(),
                                 copy.GetData());
}
void DeviceImage::CopyFromBuffer(const VkCommandBuffer commandBuffer, const DeviceBuffer &source,
                                 const TKit::Span<const VkBufferImageCopy> copy)
{
    m_Device.Table->CmdCopyBufferToImage(commandBuffer, source, m_Image, m_Layout, copy.GetSize(), copy.GetData());
}

#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_synchronization2)
void DeviceImage::CopyFromImage2(const VkCommandBuffer commandBuffer, const DeviceImage &source,
                                 const TKit::Span<const VkImageCopy2KHR> copy, const void *next)
{
    VkCopyImageInfo2KHR info{};
    info.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2_KHR;
    info.pNext = next;
    info.srcImage = m_Image;
    info.srcImageLayout = m_Layout;
    info.dstImage = source;
    info.dstImageLayout = source.GetLayout();
    info.pRegions = copy.GetData();
    info.regionCount = copy.GetSize();
    m_Device.Table->CmdCopyImage2KHR(commandBuffer, &info);
}
void DeviceImage::CopyFromBuffer2(const VkCommandBuffer commandBuffer, const DeviceBuffer &source,
                                  const TKit::Span<const VkBufferImageCopy2KHR> copy, const void *next)
{
    VkCopyBufferToImageInfo2KHR info{};
    info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2_KHR;
    info.pNext = next;
    info.srcBuffer = source;
    info.dstImage = m_Image;
    info.dstImageLayout = m_Layout;
    info.pRegions = copy.GetData();
    info.regionCount = copy.GetSize();
    m_Device.Table->CmdCopyBufferToImage2KHR(commandBuffer, &info);
}
#endif

Result<> DeviceImage::CopyFromImage(CommandPool &pool, VkQueue queue, const DeviceImage &source,
                                    const TKit::Span<const VkImageCopy> copy)
{
    const auto cres = pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromImage(cmd, source, copy);
    return pool.EndSingleTimeCommands(cmd, queue);
}

Result<> DeviceImage::CopyFromBuffer(CommandPool &pool, VkQueue queue, const DeviceBuffer &source,
                                     const TKit::Span<const VkBufferImageCopy> copy)
{
    const auto cres = pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromBuffer(cmd, source, copy);
    return pool.EndSingleTimeCommands(cmd, queue);
}

VkDeviceSize DeviceImage::GetBytesPerPixel(const VkFormat format)
{
    switch (format)
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
        TKIT_LOG_WARNING(
            "[VULKIT][DEVICE-IMAGE] Unrecognized vulkan format when resolving the number of bytes per pixel for it");
        return 0;
    }
}

VkDeviceSize DeviceImage::ComputeSize(const u32 width, const u32 height, const u32 mip, const u32 depth) const
{
    return ComputeSize(m_Info.Format, width, height, mip, depth);
}
VkDeviceSize DeviceImage::ComputeSize(const u32 mip) const
{
    return ComputeSize(m_Info.Format, m_Info.Width, m_Info.Height, mip, m_Info.Depth);
}
VkDeviceSize DeviceImage::ComputeSize(const VkFormat format, const u32 pwidth, const u32 pheight, const u32 mip,
                                      const u32 pdepth)
{
    const u32 width = Math::Max(1u, pwidth >> mip);
    const u32 height = Math::Max(1u, pheight >> mip);
    const u32 depth = Math::Max(1u, pdepth >> mip);

    const VkDeviceSize ppixel = GetBytesPerPixel(format);
    const u32 rowTexels = width;
    const u32 imgTexels = height;

    const VkDeviceSize rowStride = static_cast<VkDeviceSize>(rowTexels) * ppixel;
    const VkDeviceSize sliceStride = static_cast<VkDeviceSize>(imgTexels) * rowStride;

    return (width * ppixel + (height - 1) * rowStride + (depth - 1) * sliceStride);
}

void DeviceImage::Destroy()
{
    DestroyImageView();
    if (m_Image && m_Info.Allocation)
        vmaDestroyImage(m_Info.Allocator, m_Image, m_Info.Allocation);
    m_Info = {};
    m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
}
void DeviceImage::DestroyImageView()
{
    if (m_ImageView)
        m_Device.Table->DestroyImageView(m_Device, m_ImageView, m_Device.AllocationCallbacks);
    m_ImageView = VK_NULL_HANDLE;
}
DeviceImage::Builder &DeviceImage::Builder::SetImageType(const VkImageType type)
{
    m_ImageInfo.imageType = type;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetDepth(const u32 depth)
{
    m_ImageInfo.extent.depth = depth;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetMipLevels(const u32 levels)
{
    m_ImageInfo.mipLevels = levels;
    m_ViewInfo.subresourceRange.levelCount = levels;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetArrayLayers(const u32 layers)
{
    m_ImageInfo.arrayLayers = layers;
    m_ViewInfo.subresourceRange.layerCount = layers;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetTiling(const VkImageTiling tiling)
{
    m_ImageInfo.tiling = tiling;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetInitialLayout(const VkImageLayout layout)
{
    m_ImageInfo.initialLayout = layout;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetSamples(const VkSampleCountFlagBits samples)
{
    m_ImageInfo.samples = samples;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetSharingMode(const VkSharingMode mode)
{
    m_ImageInfo.sharingMode = mode;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetFlags(const VkImageCreateFlags flags)
{
    m_ImageInfo.flags = flags;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetUsage(const VkImageUsageFlags flags)
{
    m_ImageInfo.usage = flags;
    return *this;
}
DeviceImage::Builder &DeviceImage::Builder::SetImageCreateInfo(const VkImageCreateInfo &info)
{
    m_ImageInfo = info;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::WithImageView()
{
    m_ViewInfo.format = m_ImageInfo.format;
    return *this;
}
DeviceImage::Builder &DeviceImage::Builder::SetNextToImageInfo(const void *next)
{
    m_ImageInfo.pNext = next;
    return *this;
}
DeviceImage::Builder &DeviceImage::Builder::SetNextToImageViewInfo(const void *next)
{
    m_ViewInfo.pNext = next;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::WithImageView(const VkImageViewCreateInfo &info)
{
    TKIT_ASSERT(!info.image,
                "[VULKIT][DEVICE-IMAGE] The image must be set to null when passing a image view create info because it "
                "will be replaced with the newly created image");
    m_ViewInfo = info;
    return *this;
}
DeviceImage::Builder &DeviceImage::Builder::WithImageView(const VkImageSubresourceRange &range)
{
    m_ViewInfo.format = m_ImageInfo.format;
    m_ViewInfo.subresourceRange = range;
    return *this;
}

} // namespace VKit
