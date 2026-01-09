#include "vkit/core/pch.hpp"
#include "vkit/resource/image.hpp"
#include "vkit/resource/device_buffer.hpp"
#include "vkit/execution/command_pool.hpp"
#include "tkit/math/math.hpp"

namespace VKit
{
namespace Math = TKit::Math;
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
VkImageAspectFlags Detail::InferAspectMask(const DeviceImageFlags p_Flags)
{
    if (p_Flags & DeviceImageFlag_ColorAttachment)
        return VK_IMAGE_ASPECT_COLOR_BIT;
    else if ((p_Flags & DeviceImageFlag_DepthAttachment) && (p_Flags & DeviceImageFlag_StencilAttachment))
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    else if (p_Flags & DeviceImageFlag_DepthAttachment)
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (p_Flags & DeviceImageFlag_StencilAttachment)
        return VK_IMAGE_ASPECT_STENCIL_BIT;

    TKIT_LOG_WARNING("[VULKIT][DEVICE-IMAGE] Unable to deduce aspect mask. Using 'VK_IMAGE_ASPECT_NONE'");
    return VK_IMAGE_ASPECT_NONE;
}
static VkImageSubresourceRange createRange(const VkImageCreateInfo &p_Info, const DeviceImageFlags p_Flags)
{
    VkImageSubresourceRange range{};
    range.aspectMask |= Detail::InferAspectMask(p_Flags);

    range.baseMipLevel = 0;
    range.levelCount = p_Info.mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = p_Info.arrayLayers;
    return range;
}
DeviceImage::Builder::Builder(const ProxyDevice &p_Device, const VmaAllocator p_Allocator, const VkExtent2D &p_Extent,
                              const VkFormat p_Format, const DeviceImageFlags p_Flags)
    : DeviceImage::Builder(p_Device, p_Allocator,
                           VkExtent3D{.width = p_Extent.width, .height = p_Extent.height, .depth = 1}, p_Format,
                           p_Flags)
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

DeviceImage::Builder::Builder(const ProxyDevice &p_Device, const VmaAllocator p_Allocator, const VkExtent3D &p_Extent,
                              const VkFormat p_Format, const DeviceImageFlags p_Flags)
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
    if (p_Flags & DeviceImageFlag_ColorAttachment)
        m_ImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else if ((p_Flags & DeviceImageFlag_DepthAttachment) || (p_Flags & DeviceImageFlag_StencilAttachment))
        m_ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    if (p_Flags & DeviceImageFlag_InputAttachment)
        m_ImageInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    if (p_Flags & DeviceImageFlag_Sampled)
        m_ImageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

    m_ViewInfo = createDefaultImageViewInfo(VK_NULL_HANDLE, getImageViewType(m_ImageInfo.imageType),
                                            createRange(m_ImageInfo, p_Flags));
}

DeviceImage::Info DeviceImage::FromSwapChain(const VkFormat p_Format, const VkExtent2D &p_Extent,
                                             const DeviceImageFlags p_Flags)
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

Result<DeviceImage> DeviceImage::Builder::Build() const
{
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkImage image;
    VmaAllocation allocation;

    const VkResult result = vmaCreateImage(m_Allocator, &m_ImageInfo, &allocInfo, &image, &allocation, nullptr);
    if (result != VK_SUCCESS)
        return Result<DeviceImage>::Error(result);

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

Result<VkImageView> DeviceImage::CreateImageView(const VkImageViewCreateInfo &p_Info)
{
    const VkResult result =
        m_Device.Table->CreateImageView(m_Device, &p_Info, m_Device.AllocationCallbacks, &m_ImageView);
    if (result != VK_SUCCESS)
        return Result<VkImageView>::Error(result);
    return m_ImageView;
}

void DeviceImage::TransitionLayout(const VkCommandBuffer p_CommandBuffer, const VkImageLayout p_Layout,
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
        barrier.subresourceRange.aspectMask = Detail::InferAspectMask(m_Info.Flags);

    m_Device.Table->CmdPipelineBarrier(p_CommandBuffer, p_Info.SrcStage, p_Info.DstStage, 0, 0, nullptr, 0, nullptr, 1,
                                       &barrier);
    m_Layout = p_Layout;
}

void DeviceImage::CopyFromImage(const VkCommandBuffer p_CommandBuffer, const DeviceImage &p_Source,
                                const TKit::Span<const VkImageCopy> p_Copy)
{
    m_Device.Table->CmdCopyImage(p_CommandBuffer, p_Source, p_Source.GetLayout(), m_Image, m_Layout, p_Copy.GetSize(),
                                 p_Copy.GetData());
}
void DeviceImage::CopyFromBuffer(const VkCommandBuffer p_CommandBuffer, const DeviceBuffer &p_Source,
                                 const TKit::Span<const VkBufferImageCopy> p_Copy)
{
    m_Device.Table->CmdCopyBufferToImage(p_CommandBuffer, p_Source, m_Image, m_Layout, p_Copy.GetSize(),
                                         p_Copy.GetData());
}

#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_synchronization2)
void DeviceImage::CopyFromImage2(const VkCommandBuffer p_CommandBuffer, const DeviceImage &p_Source,
                                 const TKit::Span<const VkImageCopy2KHR> p_Copy, const void *p_Next)
{
    VkCopyImageInfo2KHR info{};
    info.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2_KHR;
    info.pNext = p_Next;
    info.srcImage = m_Image;
    info.srcImageLayout = m_Layout;
    info.dstImage = p_Source;
    info.dstImageLayout = p_Source.GetLayout();
    info.pRegions = p_Copy.GetData();
    info.regionCount = p_Copy.GetSize();
    m_Device.Table->CmdCopyImage2KHR(p_CommandBuffer, &info);
}
void DeviceImage::CopyFromBuffer2(const VkCommandBuffer p_CommandBuffer, const DeviceBuffer &p_Source,
                                  const TKit::Span<const VkBufferImageCopy2KHR> p_Copy, const void *p_Next)
{
    VkCopyBufferToImageInfo2KHR info{};
    info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2_KHR;
    info.pNext = p_Next;
    info.srcBuffer = p_Source;
    info.dstImage = m_Image;
    info.dstImageLayout = m_Layout;
    info.pRegions = p_Copy.GetData();
    info.regionCount = p_Copy.GetSize();
    m_Device.Table->CmdCopyBufferToImage2KHR(p_CommandBuffer, &info);
}
#endif

Result<> DeviceImage::CopyFromImage(CommandPool &p_Pool, VkQueue p_Queue, const DeviceImage &p_Source,
                                    const TKit::Span<const VkImageCopy> p_Copy)
{
    const auto cres = p_Pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromImage(cmd, p_Source, p_Copy);
    return p_Pool.EndSingleTimeCommands(cmd, p_Queue);
}

Result<> DeviceImage::CopyFromBuffer(CommandPool &p_Pool, VkQueue p_Queue, const DeviceBuffer &p_Source,
                                     const TKit::Span<const VkBufferImageCopy> p_Copy)
{
    const auto cres = p_Pool.BeginSingleTimeCommands();
    TKIT_RETURN_ON_ERROR(cres);

    const VkCommandBuffer cmd = cres.GetValue();
    CopyFromBuffer(cmd, p_Source, p_Copy);
    return p_Pool.EndSingleTimeCommands(cmd, p_Queue);
}

VkDeviceSize DeviceImage::GetBytesPerPixel(const VkFormat p_Format)
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
        TKIT_LOG_WARNING(
            "[VULKIT][DEVICE-IMAGE] Unrecognized vulkan format when resolving the number of bytes per pixel for it");
        return 0;
    }
    TKIT_LOG_WARNING(
        "[VULKIT][DEVICE-IMAGE] Unrecognized vulkan format when resolving the number of bytes per pixel for it");
    return 0;
}

VkDeviceSize DeviceImage::ComputeSize(const u32 p_Width, const u32 p_Height, const u32 p_Mip, const u32 p_Depth) const
{
    return ComputeSize(m_Info.Format, p_Width, p_Height, p_Mip, p_Depth);
}
VkDeviceSize DeviceImage::ComputeSize(const u32 p_Mip) const
{
    return ComputeSize(m_Info.Format, m_Info.Width, m_Info.Height, p_Mip, m_Info.Depth);
}
VkDeviceSize DeviceImage::ComputeSize(const VkFormat p_Format, const u32 p_Width, const u32 p_Height, const u32 p_Mip,
                                      const u32 p_Depth)
{
    const u32 width = Math::Max(1u, p_Width >> p_Mip);
    const u32 height = Math::Max(1u, p_Height >> p_Mip);
    const u32 depth = Math::Max(1u, p_Depth >> p_Mip);

    const VkDeviceSize ppixel = GetBytesPerPixel(p_Format);
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
DeviceImage::Builder &DeviceImage::Builder::SetImageType(const VkImageType p_Type)
{
    m_ImageInfo.imageType = p_Type;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetDepth(const u32 p_Depth)
{
    m_ImageInfo.extent.depth = p_Depth;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetMipLevels(const u32 p_Levels)
{
    m_ImageInfo.mipLevels = p_Levels;
    m_ViewInfo.subresourceRange.levelCount = p_Levels;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetArrayLayers(const u32 p_Layers)
{
    m_ImageInfo.arrayLayers = p_Layers;
    m_ViewInfo.subresourceRange.layerCount = p_Layers;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetTiling(const VkImageTiling p_Tiling)
{
    m_ImageInfo.tiling = p_Tiling;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetInitialLayout(const VkImageLayout p_Layout)
{
    m_ImageInfo.initialLayout = p_Layout;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetSamples(const VkSampleCountFlagBits p_Samples)
{
    m_ImageInfo.samples = p_Samples;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetSharingMode(const VkSharingMode p_Mode)
{
    m_ImageInfo.sharingMode = p_Mode;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetFlags(const VkImageCreateFlags p_Flags)
{
    m_ImageInfo.flags = p_Flags;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::SetUsage(const VkImageUsageFlags p_Flags)
{
    m_ImageInfo.usage = p_Flags;
    return *this;
}
DeviceImage::Builder &DeviceImage::Builder::SetImageCreateInfo(const VkImageCreateInfo &p_Info)
{
    m_ImageInfo = p_Info;
    return *this;
}

DeviceImage::Builder &DeviceImage::Builder::WithImageView()
{
    m_ViewInfo.format = m_ImageInfo.format;
    return *this;
}
DeviceImage::Builder &DeviceImage::Builder::WithImageView(const VkImageViewCreateInfo &p_Info)
{
    TKIT_ASSERT(!p_Info.image,
                "[VULKIT][DEVICE-IMAGE] The image must be set to null when passing a image view create info because it "
                "will be replaced with the newly created image");
    m_ViewInfo = p_Info;
    return *this;
}
DeviceImage::Builder &DeviceImage::Builder::WithImageView(const VkImageSubresourceRange &p_Range)
{
    m_ViewInfo.format = m_ImageInfo.format;
    m_ViewInfo.subresourceRange = p_Range;
    return *this;
}

} // namespace VKit
