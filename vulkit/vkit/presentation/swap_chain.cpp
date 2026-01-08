#include "vkit/core/pch.hpp"
#include "vkit/presentation/swap_chain.hpp"

#ifndef VK_KHR_surface
#    error "[VULKIT] To use the swap chain abstraction, the vulkan headers must support the surface extension"
#else

namespace VKit
{
Result<VkSurfaceFormatKHR> selectFormat(const TKit::Span<const VkSurfaceFormatKHR> p_Requested,
                                        const TKit::Span<const VkSurfaceFormatKHR> p_Supported)
{
    for (const VkSurfaceFormatKHR &desired : p_Requested)
        for (const VkSurfaceFormatKHR &supported : p_Supported)
            if (desired.format == supported.format && desired.colorSpace == supported.colorSpace)
                return Result<VkSurfaceFormatKHR>::Ok(desired);
    return Result<VkSurfaceFormatKHR>::Error(Error_NoFormatSupported);
}

Result<VkPresentModeKHR> selectPresentMode(const TKit::Span<const VkPresentModeKHR> p_Requested,
                                           const TKit::Span<const VkPresentModeKHR> p_Supported)
{
    for (const VkPresentModeKHR &desired : p_Requested)
        for (const VkPresentModeKHR &supported : p_Supported)
            if (desired == supported)
                return desired;
    return Result<VkPresentModeKHR>::Error(Error_NoFormatSupported);
}

Result<SwapChain> SwapChain::Builder::Build() const
{
    const ProxyDevice proxy = m_Device->CreateProxy();
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkCreateSwapchainKHR, Result<SwapChain>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkDestroySwapchainKHR, Result<SwapChain>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkGetSwapchainImagesKHR, Result<SwapChain>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkCreateImageView, Result<SwapChain>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkDestroyImageView, Result<SwapChain>);

    const PhysicalDevice::Info &devInfo = m_Device->GetInfo().PhysicalDevice.GetInfo();
    if (devInfo.FamilyIndices[Queue_Graphics] == TKIT_U32_MAX || devInfo.FamilyIndices[Queue_Present] == TKIT_U32_MAX)
        return Result<SwapChain>::Error(Error_MissingQueue);

    const auto checkFlags = [this](const SwapChainBuilderFlags p_Flags) -> bool { return m_Flags & p_Flags; };

    TKit::Array16<VkSurfaceFormatKHR> imageFormats = m_SurfaceFormats;
    TKit::Array8<VkPresentModeKHR> presentModes = m_PresentModes;
    if (imageFormats.IsEmpty())
        imageFormats.Append(VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});
    if (presentModes.IsEmpty())
    {
        presentModes.Append(VK_PRESENT_MODE_MAILBOX_KHR);
        presentModes.Append(VK_PRESENT_MODE_FIFO_KHR);
    }

    const auto suppResult = m_Device->QuerySwapChainSupport(m_Surface);
    TKIT_RETURN_ON_ERROR(suppResult);

    const PhysicalDevice::SwapChainSupportDetails &support = suppResult.GetValue();

    const u32 mnic = support.Capabilities.minImageCount;
    const u32 mxic = support.Capabilities.maxImageCount;
    const auto badImageCount = [=](const u32 p_Count) -> const char * {
        if (p_Count < mnic)
            return "The requested image count is less than the minimum image count";
        if (mxic > 0 && p_Count > mxic)
            return "The requested image count is greater than the maximum image count";
        return nullptr;
    };

    u32 imageCount = m_RequestedImages;
    const char *err = badImageCount(imageCount);
    if (err)
    {
        TKIT_LOG_WARNING("[VULKIT] {}", err);
        if (m_RequiredImages == 0)
        {
            imageCount = mnic + 1;
            if (mxic > 0 && imageCount > mxic)
                imageCount = mxic;
        }
        else
        {
            imageCount = m_RequiredImages;
            const char *error = badImageCount(imageCount);
            if (error)
                return Result<SwapChain>::Error(Error_BadImageCount, error);
        }
    }

    const auto surfFormatResult = selectFormat(imageFormats, support.Formats);
    TKIT_RETURN_ON_ERROR(surfFormatResult);

    const auto presentModeResult = selectPresentMode(presentModes, support.PresentModes);
    TKIT_RETURN_ON_ERROR(presentModeResult);

    const VkSurfaceFormatKHR surfaceFormat = surfFormatResult.GetValue();
    const VkPresentModeKHR presentMode = presentModeResult.GetValue();

    VkExtent2D extent;
    if (support.Capabilities.currentExtent.width != TKIT_U32_MAX &&
        support.Capabilities.currentExtent.height != TKIT_U32_MAX)
        extent = support.Capabilities.currentExtent;
    else
    {
        extent.width = std::clamp(m_Extent.width, support.Capabilities.minImageExtent.width,
                                  support.Capabilities.maxImageExtent.width);
        extent.height = std::clamp(m_Extent.height, support.Capabilities.minImageExtent.height,
                                   support.Capabilities.maxImageExtent.height);
    }
    VkSurfaceTransformFlagBitsKHR transform = m_TransformBit;
    if (transform == static_cast<VkSurfaceTransformFlagBitsKHR>(0))
        transform = support.Capabilities.currentTransform;

    const TKit::FixedArray<u32, 2> indices{devInfo.FamilyIndices[Queue_Graphics], devInfo.FamilyIndices[Queue_Present]};
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = m_ImageArrayLayers;
    createInfo.imageUsage = m_ImageUsage;
    if (devInfo.FamilyIndices[Queue_Graphics] != devInfo.FamilyIndices[Queue_Present])
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = indices.GetData();
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform = transform;
    createInfo.compositeAlpha = m_CompositeAlphaFlags;
    createInfo.presentMode = presentMode;
    createInfo.clipped = checkFlags(SwapChainBuilderFlag_Clipped);
    createInfo.oldSwapchain = m_OldSwapChain;
    createInfo.flags = m_CreateFlags;

    VkSwapchainKHR swapChain;
    VkResult result = proxy.Table->CreateSwapchainKHR(proxy, &createInfo, proxy.AllocationCallbacks, &swapChain);
    if (result != VK_SUCCESS)
        return Result<SwapChain>::Error(result);

    TKit::Array8<DeviceImage> finalImages;
    SwapChain::Info info{};
    info.Extent = extent;
    info.SurfaceFormat = surfaceFormat;
    info.PresentMode = presentMode;
    info.ImageUsage = m_ImageUsage;
    info.Flags = m_Flags;
    info.SupportDetails = support;

    const auto earlyDestroy = [proxy, swapChain, &checkFlags, &finalImages] {
        if (checkFlags(SwapChainBuilderFlag_CreateImageViews))
            for (DeviceImage &image : finalImages)
                image.DestroyImageView();

        proxy.Table->DestroySwapchainKHR(proxy, swapChain, proxy.AllocationCallbacks);
    };

    result = proxy.Table->GetSwapchainImagesKHR(proxy, swapChain, &imageCount, nullptr);
    if (result != VK_SUCCESS)
    {
        earlyDestroy();
        return Result<SwapChain>::Error(result);
    }

    TKit::Array8<VkImage> images;
    images.Resize(imageCount);

    result = proxy.Table->GetSwapchainImagesKHR(proxy, swapChain, &imageCount, images.GetData());
    if (result != VK_SUCCESS)
    {
        earlyDestroy();
        return Result<SwapChain>::Error(result);
    }

    finalImages.Resize(imageCount);
    for (u32 i = 0; i < imageCount; ++i)
    {
        DeviceImage::Info iminfo = DeviceImage::FromSwapChain(surfaceFormat.format, extent);
        VkImageView view = VK_NULL_HANDLE;
        if (checkFlags(SwapChainBuilderFlag_CreateImageViews))
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = images[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = surfaceFormat.format;
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            result = proxy.Table->CreateImageView(proxy, &viewInfo, proxy.AllocationCallbacks, &view);
            if (result != VK_SUCCESS)
            {
                earlyDestroy();
                return Result<SwapChain>::Error(result);
            }
        }
        finalImages[i] = DeviceImage{*m_Device, images[i], VK_IMAGE_LAYOUT_UNDEFINED, iminfo, view};
    }

    return Result<SwapChain>::Ok(proxy, swapChain, finalImages, info);
}

void SwapChain::Destroy()
{
    for (DeviceImage image : m_Images)
        image.DestroyImageView();

    m_Images.Clear();

    if (m_SwapChain)
    {
        m_Device.Table->DestroySwapchainKHR(m_Device, m_SwapChain, m_Device.AllocationCallbacks);
        m_SwapChain = VK_NULL_HANDLE;
    }
}
SwapChain::Builder &SwapChain::Builder::RequestSurfaceFormat(const VkSurfaceFormatKHR p_Format)
{
    m_SurfaceFormats.Insert(m_SurfaceFormats.begin(), p_Format);
    return *this;
}
SwapChain::Builder &SwapChain::Builder::AllowSurfaceFormat(const VkSurfaceFormatKHR p_Format)
{
    m_SurfaceFormats.Append(p_Format);
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RequestPresentMode(const VkPresentModeKHR p_Mode)
{
    m_PresentModes.Insert(m_PresentModes.begin(), p_Mode);
    return *this;
}
SwapChain::Builder &SwapChain::Builder::AllowPresentMode(const VkPresentModeKHR p_Mode)
{
    m_PresentModes.Append(p_Mode);
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RequestImageCount(const u32 p_Images)
{
    m_RequestedImages = p_Images;
    if (m_RequestedImages < m_RequiredImages)
        m_RequiredImages = m_RequestedImages;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RequireImageCount(const u32 p_Images)
{
    m_RequiredImages = p_Images;
    if (m_RequestedImages < m_RequiredImages)
        m_RequestedImages = m_RequiredImages;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RequestExtent(const u32 p_Width, const u32 p_Height)
{
    m_Extent.width = p_Width;
    m_Extent.height = p_Height;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RequestExtent(const VkExtent2D &p_Extent)
{
    m_Extent = p_Extent;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetFlags(const SwapChainBuilderFlags p_Flags)
{
    m_Flags = p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::AddFlags(const SwapChainBuilderFlags p_Flags)
{
    m_Flags |= p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RemoveFlags(const SwapChainBuilderFlags p_Flags)
{
    m_Flags &= ~p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetImageArrayLayers(const u32 p_Layers)
{
    m_ImageArrayLayers = p_Layers;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetCreateFlags(const VkSwapchainCreateFlagsKHR p_Flags)
{
    m_CreateFlags = p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::AddCreateFlags(const VkSwapchainCreateFlagsKHR p_Flags)
{
    m_CreateFlags |= p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RemoveCreateFlags(const VkSwapchainCreateFlagsKHR p_Flags)
{
    m_CreateFlags &= ~p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetImageUsageFlags(const VkImageUsageFlags p_Flags)
{
    m_ImageUsage = p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::AddImageUsageFlags(const VkImageUsageFlags p_Flags)
{
    m_ImageUsage |= p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RemoveImageUsageFlags(const VkImageUsageFlags p_Flags)
{
    m_ImageUsage &= ~p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetTransformBit(const VkSurfaceTransformFlagBitsKHR p_Transform)
{
    m_TransformBit = p_Transform;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetCompositeAlphaBit(const VkCompositeAlphaFlagBitsKHR p_Alpha)
{
    m_CompositeAlphaFlags = p_Alpha;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetOldSwapChain(const VkSwapchainKHR p_OldSwapChain)
{
    m_OldSwapChain = p_OldSwapChain;
    return *this;
}

} // namespace VKit
#endif
