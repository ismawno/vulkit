#include "vkit/core/pch.hpp"
#include "vkit/rendering/swap_chain.hpp"

namespace VKit
{

Result<VkSurfaceFormatKHR> selectFormat(const TKit::Span<const VkSurfaceFormatKHR> p_Requested,
                                        const TKit::Span<const VkSurfaceFormatKHR> p_Supported)
{
    for (const VkSurfaceFormatKHR &desired : p_Requested)
        for (const VkSurfaceFormatKHR &supported : p_Supported)
            if (desired.format == supported.format && desired.colorSpace == supported.colorSpace)
                return Result<VkSurfaceFormatKHR>::Ok(desired);
    return Result<VkSurfaceFormatKHR>::Error(VK_ERROR_FORMAT_NOT_SUPPORTED,
                                             "No desired format that is supported found");
}

Result<VkPresentModeKHR> selectPresentMode(const TKit::Span<const VkPresentModeKHR> p_Requested,
                                           const TKit::Span<const VkPresentModeKHR> p_Supported)
{
    for (const VkPresentModeKHR &desired : p_Requested)
        for (const VkPresentModeKHR &supported : p_Supported)
            if (desired == supported)
                return Result<VkPresentModeKHR>::Ok(desired);
    return Result<VkPresentModeKHR>::Error(VK_ERROR_FORMAT_NOT_SUPPORTED,
                                           "No desired present mode that is supported found");
}

Result<SwapChain> SwapChain::Builder::Build() const
{
    const LogicalDevice::Proxy proxy = m_Device->CreateProxy();
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkCreateSwapchainKHR, Result<SwapChain>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkDestroySwapchainKHR, Result<SwapChain>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkGetSwapchainImagesKHR, Result<SwapChain>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkCreateImageView, Result<SwapChain>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(proxy.Table, vkDestroyImageView, Result<SwapChain>);

    const PhysicalDevice::Info &devInfo = m_Device->GetPhysicalDevice().GetInfo();
    if (devInfo.GraphicsIndex == UINT32_MAX)
        return Result<SwapChain>::Error(VK_ERROR_INITIALIZATION_FAILED, "No graphics queue found");
    if (devInfo.PresentIndex == UINT32_MAX)
        return Result<SwapChain>::Error(VK_ERROR_INITIALIZATION_FAILED, "No present queue found");

    const auto checkFlag = [this](const FlagBits p_Flag) -> bool { return m_Flags & p_Flag; };

    TKit::StaticArray16<VkSurfaceFormatKHR> imageFormats = m_SurfaceFormats;
    TKit::StaticArray8<VkPresentModeKHR> presentModes = m_PresentModes;
    if (imageFormats.IsEmpty())
        imageFormats.Append(VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});
    if (presentModes.IsEmpty())
    {
        presentModes.Append(VK_PRESENT_MODE_MAILBOX_KHR);
        presentModes.Append(VK_PRESENT_MODE_FIFO_KHR);
    }

    const auto suppResult = m_Device->QuerySwapChainSupport(m_Surface);
    if (!suppResult)
        return Result<SwapChain>::Error(suppResult.GetError());

    const PhysicalDevice::SwapChainSupportDetails &support = suppResult.GetValue();

    const auto badImageCount = [&support](const u32 p_Count) -> const char * {
        if (p_Count < support.Capabilities.minImageCount)
            return "The requested image count is less than the minimum image count";
        if (support.Capabilities.maxImageCount > 0 && p_Count > support.Capabilities.maxImageCount)
            return "The requested image count is greater than the maximum image count";
        return nullptr;
    };

    u32 minImageCount = m_RequestedImages;
    if (badImageCount(minImageCount))
    {
        if (m_RequiredImages == 0)
        {
            minImageCount = support.Capabilities.minImageCount + 1;
            if (minImageCount > support.Capabilities.maxImageCount)
                minImageCount = support.Capabilities.maxImageCount;
        }
        else
        {
            minImageCount = m_RequiredImages;
            const char *error = badImageCount(minImageCount);
            if (error)
                return Result<SwapChain>::Error(VK_ERROR_INITIALIZATION_FAILED, error);
        }
    }

    const auto surfFormatResult = selectFormat(imageFormats, support.Formats);
    if (!surfFormatResult)
        return Result<SwapChain>::Error(surfFormatResult.GetError());

    const auto presentModeResult = selectPresentMode(presentModes, support.PresentModes);
    if (!presentModeResult)
        return Result<SwapChain>::Error(presentModeResult.GetError());

    const VkSurfaceFormatKHR surfaceFormat = surfFormatResult.GetValue();
    const VkPresentModeKHR presentMode = presentModeResult.GetValue();

    VkExtent2D extent;
    if (support.Capabilities.currentExtent.width != UINT32_MAX &&
        support.Capabilities.currentExtent.height != UINT32_MAX)
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

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = minImageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = m_ImageArrayLayers;
    createInfo.imageUsage = m_ImageUsage;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (devInfo.GraphicsIndex != devInfo.PresentIndex)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = &devInfo.GraphicsIndex;
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
    createInfo.clipped = checkFlag(Flag_Clipped);
    createInfo.oldSwapchain = m_OldSwapChain;
    createInfo.flags = m_CreateFlags;

    VkSwapchainKHR swapChain;
    VkResult result = proxy.Table->CreateSwapchainKHR(proxy, &createInfo, proxy.AllocationCallbacks, &swapChain);
    if (result != VK_SUCCESS)
        return Result<SwapChain>::Error(result, "Failed to create the swap chain");

    SwapChain::Info info{};
    info.Extent = extent;
    info.SurfaceFormat = surfaceFormat;
    info.PresentMode = presentMode;
    info.ImageUsage = m_ImageUsage;
    info.Flags = m_Flags;
    info.SupportDetails = support;

    const auto earlyDestroy = [proxy, swapChain, &checkFlag, &info] {
        if (checkFlag(Flag_CreateImageViews))
            for (const ImageData &data : info.ImageData)
                if (data.ImageView)
                    proxy.Table->DestroyImageView(proxy, data.ImageView, proxy.AllocationCallbacks);

        proxy.Table->DestroySwapchainKHR(proxy, swapChain, proxy.AllocationCallbacks);
    };

    u32 imageCount;
    result = proxy.Table->GetSwapchainImagesKHR(proxy, swapChain, &imageCount, nullptr);
    if (result != VK_SUCCESS)
    {
        earlyDestroy();
        return Result<SwapChain>::Error(result, "Failed to get the swap chain images count");
    }

    TKit::StaticArray8<VkImage> images;
    images.Resize(imageCount);

    result = proxy.Table->GetSwapchainImagesKHR(proxy, swapChain, &imageCount, images.GetData());
    if (result != VK_SUCCESS)
    {
        earlyDestroy();
        return Result<SwapChain>::Error(result, "Failed to get the swap chain images");
    }

    info.ImageData.Resize(imageCount);
    for (u32 i = 0; i < imageCount; ++i)
    {
        info.ImageData[i].Image = images[i];
        if (checkFlag(Flag_CreateImageViews))
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

            result =
                proxy.Table->CreateImageView(proxy, &viewInfo, proxy.AllocationCallbacks, &info.ImageData[i].ImageView);
            if (result != VK_SUCCESS)
            {
                earlyDestroy();
                return Result<SwapChain>::Error(result, "Failed to create the image view");
            }
        }
    }

    return Result<SwapChain>::Ok(proxy, swapChain, info);
}

void SwapChain::destroy() const
{
    TKIT_ASSERT(m_SwapChain, "[VULKIT] The swap chain is a NULL handle");

    if (m_Info.Flags & Flag_HasImageViews)
        for (const ImageData &data : m_Info.ImageData)
            m_Device.Table->DestroyImageView(m_Device, data.ImageView, m_Device.AllocationCallbacks);

    m_Device.Table->DestroySwapchainKHR(m_Device, m_SwapChain, m_Device.AllocationCallbacks);
}

void SwapChain::Destroy()
{
    destroy();
    m_SwapChain = VK_NULL_HANDLE;
}
void SwapChain::SubmitForDeletion(DeletionQueue &p_Queue) const
{
    const SwapChain swapChain = *this;
    p_Queue.Push([swapChain] { swapChain.destroy(); }); // That is stupid...
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
SwapChain::Builder &SwapChain::Builder::SetFlags(const Flags p_Flags)
{
    m_Flags = p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::AddFlags(const Flags p_Flags)
{
    m_Flags |= p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RemoveFlags(const Flags p_Flags)
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
