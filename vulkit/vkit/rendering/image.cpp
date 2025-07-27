#include "vkit/core/pch.hpp"
#include "vkit/rendering/image.hpp"

namespace VKit
{
Result<ImageHouse> ImageHouse::Create(const LogicalDevice::Proxy &p_Device, const VmaAllocator p_Allocator) noexcept
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCreateImageView, Result<ImageHouse>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkDestroyImageView, Result<ImageHouse>);

    return Result<ImageHouse>::Ok(p_Device, p_Allocator);
}
Result<Image> ImageHouse::CreateImage(const VkImageCreateInfo &p_Info, const VkImageSubresourceRange &p_Range,
                                      const VkImageViewType p_ViewType) const noexcept
{
    if (!m_Allocator)
        return Result<Image>::Error(VK_ERROR_INITIALIZATION_FAILED, "An allocator must be set to create resources");

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    Image imageData;
    VkResult result =
        vmaCreateImage(m_Allocator, &p_Info, &allocInfo, &imageData.Image, &imageData.Allocation, nullptr);
    if (result != VK_SUCCESS)
        return Result<Image>::Error(result, "Failed to create image");

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = imageData.Image;
    viewInfo.viewType = p_ViewType;
    viewInfo.format = p_Info.format;
    viewInfo.subresourceRange = p_Range;

    result = m_Device.Table->CreateImageView(m_Device, &viewInfo, m_Device.AllocationCallbacks, &imageData.ImageView);
    if (result != VK_SUCCESS)
    {
        vmaDestroyImage(m_Allocator, imageData.Image, imageData.Allocation);
        return Result<Image>::Error(result, "Failed to create image view");
    }
    return Result<Image>::Ok(imageData);
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
Result<Image> ImageHouse::CreateImage(const VkImageCreateInfo &p_Info,
                                      const VkImageSubresourceRange &p_Range) const noexcept
{
    const VkImageViewType viewType = getImageViewType(p_Info.imageType);
    if (viewType == VK_IMAGE_VIEW_TYPE_MAX_ENUM)
        return Result<Image>::Error(VK_ERROR_INITIALIZATION_FAILED, "Invalid image type");
    return CreateImage(p_Info, p_Range, viewType);
}
static Result<VkImageSubresourceRange> getRange(const VkImageCreateInfo &p_Info, const AttachmentFlags p_Flags) noexcept
{
    VkImageSubresourceRange range{};
    if (p_Flags & AttachmentFlag_Color)
        range.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
    else if (p_Flags & AttachmentFlag_Depth)
        range.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (p_Flags & AttachmentFlag_Stencil)
        range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    else
        return Result<VkImageSubresourceRange>::Error(VK_ERROR_INITIALIZATION_FAILED, "Invalid attachment type");

    range.baseMipLevel = 0;
    range.levelCount = p_Info.mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = p_Info.arrayLayers;
    return Result<VkImageSubresourceRange>::Ok(range);
}
Result<Image> ImageHouse::CreateImage(const VkImageCreateInfo &p_Info, const VkImageViewType p_ViewType,
                                      const AttachmentFlags p_Flags) const noexcept
{
    const auto rangeResult = getRange(p_Info, p_Flags);
    if (!rangeResult)
        return Result<Image>::Error(rangeResult.GetError());

    const VkImageSubresourceRange &range = rangeResult.GetValue();
    return CreateImage(p_Info, range, p_ViewType);
}
Result<Image> ImageHouse::CreateImage(const VkImageCreateInfo &p_Info, const AttachmentFlags p_Flags) const noexcept
{
    const VkImageViewType viewType = getImageViewType(p_Info.imageType);
    if (viewType == VK_IMAGE_VIEW_TYPE_MAX_ENUM)
        return Result<Image>::Error(VK_ERROR_INITIALIZATION_FAILED, "Invalid image type");

    const auto rangeResult = getRange(p_Info, p_Flags);
    if (!rangeResult)
        return Result<Image>::Error(rangeResult.GetError());

    const VkImageSubresourceRange &range = rangeResult.GetValue();
    return CreateImage(p_Info, range, viewType);
}
Result<Image> ImageHouse::CreateImage(const VkFormat p_Format, const VkExtent2D &p_Extent,
                                      const AttachmentFlags p_Flags) const noexcept
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = p_Extent.width;
    imageInfo.extent.height = p_Extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = p_Format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = 0;
    if (p_Flags & AttachmentFlag_Color)
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else if ((p_Flags & AttachmentFlag_Depth) || (p_Flags & AttachmentFlag_Stencil))
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    if (p_Flags & AttachmentFlag_Input)
        imageInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    if (p_Flags & AttachmentFlag_Sampled)
        imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

    return CreateImage(imageInfo, p_Flags);
}
Result<Image> ImageHouse::CreateImage(const VkImageView p_ImageView) const noexcept
{
    Image imageData{};
    imageData.Image = VK_NULL_HANDLE;
    imageData.Allocation = VK_NULL_HANDLE;
    imageData.ImageView = p_ImageView;
    return Result<Image>::Ok(imageData);
}

const LogicalDevice::Proxy &ImageHouse::GetDevice() const noexcept
{
    return m_Device;
}

void ImageHouse::DestroyImage(const Image &p_Image) const noexcept
{
    if (!p_Image.Image)
        return;

    vmaDestroyImage(m_Allocator, p_Image.Image, p_Image.Allocation);
    m_Device.Table->DestroyImageView(m_Device, p_Image.ImageView, m_Device.AllocationCallbacks);
}
void ImageHouse::SubmitImageForDeletion(const Image &p_Image, DeletionQueue &p_Queue) const noexcept
{
    p_Queue.Push([this, p_Image]() { DestroyImage(p_Image); });
}

} // namespace VKit
