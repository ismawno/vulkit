#include "vkit/core/pch.hpp"
#include "vkit/presentation/swap_chain.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/utils/limits.hpp"

#ifndef VK_KHR_surface
#    error "[VULKIT] To use the swap chain abstraction, the vulkan headers must support the surface extension"
#else

namespace VKit
{
static Result<VkSurfaceFormatKHR> selectFormat(const TKit::Span<const VkSurfaceFormatKHR> requested,
                                               const TKit::Span<const VkSurfaceFormatKHR> supported)
{
    for (const VkSurfaceFormatKHR &desired : requested)
        for (const VkSurfaceFormatKHR &supp : supported)
            if (desired.format == supp.format && desired.colorSpace == supp.colorSpace)
                return Result<VkSurfaceFormatKHR>::Ok(desired);
    return Result<VkSurfaceFormatKHR>::Error(Error_NoFormatSupported);
}

static Result<VkPresentModeKHR> selectPresentMode(const TKit::Span<const VkPresentModeKHR> requested,
                                                  const TKit::Span<const VkPresentModeKHR> supported)
{
    for (const VkPresentModeKHR &desired : requested)
        for (const VkPresentModeKHR &supp : supported)
            if (desired == supp)
                return desired;
    return Result<VkPresentModeKHR>::Error(Error_NoFormatSupported);
}

Result<Swapchain> Swapchain::Builder::Build() const
{
    const ProxyDevice proxy = m_Device->CreateProxy();
    const PhysicalDevice::Info &devInfo = m_Device->GetInfo().PhysicalDevice->GetInfo();
    if (devInfo.FamilyIndices[Queue_Graphics] == TKIT_U32_MAX || devInfo.FamilyIndices[Queue_Present] == TKIT_U32_MAX)
        return Result<Swapchain>::Error(Error_MissingQueue);

    const auto checkFlags = [this](const SwapchainBuilderFlags flags) -> bool { return m_Flags & flags; };

    TKit::StackArray<VkSurfaceFormatKHR> imageFormats;
    imageFormats.Reserve(m_SurfaceFormats.GetCapacity() + 1);
    imageFormats = m_SurfaceFormats;

    TKit::StackArray<VkPresentModeKHR> presentModes;
    presentModes.Reserve(presentModes.GetCapacity() + 2);
    presentModes = m_PresentModes;

    if (imageFormats.IsEmpty())
        imageFormats.Append(VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});
    if (presentModes.IsEmpty())
    {
        presentModes.Append(VK_PRESENT_MODE_MAILBOX_KHR);
        presentModes.Append(VK_PRESENT_MODE_FIFO_KHR);
    }

    const auto suppResult = m_Device->QuerySwapchainSupport(m_Surface);
    TKIT_RETURN_ON_ERROR(suppResult);

    const PhysicalDevice::SwapchainSupportDetails &support = *suppResult;

    const u32 mnic = support.Capabilities.minImageCount;
    const u32 mxic = support.Capabilities.maxImageCount;
    const auto badImageCount = [=](const u32 count) -> const char * {
        if (count < mnic)
            return "The requested image count is less than the minimum image count";
        if (mxic > 0 && count > mxic)
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
                return Result<Swapchain>::Error(Error_BadImageCount, error);
        }
    }

    const auto surfFormatResult = selectFormat(imageFormats, support.Formats);
    TKIT_RETURN_ON_ERROR(surfFormatResult);

    const auto presentModeResult = selectPresentMode(presentModes, support.PresentModes);
    TKIT_RETURN_ON_ERROR(presentModeResult);

    const VkSurfaceFormatKHR surfaceFormat = *surfFormatResult;
    const VkPresentModeKHR presentMode = *presentModeResult;

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
    if (transform == VkSurfaceTransformFlagBitsKHR(0))
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
    createInfo.clipped = checkFlags(SwapchainBuilderFlag_Clipped);
    createInfo.oldSwapchain = m_OldSwapchain;
    createInfo.flags = m_CreateFlags;

    VkSwapchainKHR swapChain;
    VKIT_RETURN_IF_FAILED(proxy.Table->CreateSwapchainKHR(proxy, &createInfo, proxy.AllocationCallbacks, &swapChain),
                          Result<Swapchain>);

    TKit::TierArray<DeviceImage> finalImages;
    Swapchain::Info info{};
    info.Extent = extent;
    info.SurfaceFormat = surfaceFormat;
    info.PresentMode = presentMode;
    info.ImageUsage = m_ImageUsage;
    info.Flags = m_Flags;
    info.SupportDetails = support;

    const auto cleanup = [proxy, swapChain, &checkFlags, &finalImages] {
        if (checkFlags(SwapchainBuilderFlag_CreateImageViews))
            for (DeviceImage &image : finalImages)
                image.DestroyImageViews();

        proxy.Table->DestroySwapchainKHR(proxy, swapChain, proxy.AllocationCallbacks);
    };

    VKIT_RETURN_IF_FAILED(proxy.Table->GetSwapchainImagesKHR(proxy, swapChain, &imageCount, nullptr), Result<Swapchain>,
                          cleanup());

    TKit::StackArray<VkImage> images;
    images.Resize(imageCount);

    VKIT_RETURN_IF_FAILED(proxy.Table->GetSwapchainImagesKHR(proxy, swapChain, &imageCount, images.GetData()),
                          Result<Swapchain>, cleanup());

    finalImages.Resize(imageCount);
    for (u32 i = 0; i < imageCount; ++i)
    {
        DeviceImage::Info iminfo = DeviceImage::Info::FromSwapchain(surfaceFormat.format, extent);
        finalImages[i] = DeviceImage{*m_Device, images[i], VK_IMAGE_LAYOUT_UNDEFINED, iminfo};
        if (checkFlags(SwapchainBuilderFlag_CreateImageViews))
            TKIT_RETURN_IF_FAILED(finalImages[i].AddImageView(), cleanup());
    }

    return Result<Swapchain>::Ok(proxy, swapChain, finalImages, info);
}

void Swapchain::Destroy()
{
    for (DeviceImage image : m_Images)
        image.DestroyImageViews();

    m_Images.Clear();

    if (m_Swapchain)
    {
        m_Device.Table->DestroySwapchainKHR(m_Device, m_Swapchain, m_Device.AllocationCallbacks);
        m_Swapchain = VK_NULL_HANDLE;
    }
}
Swapchain::Builder &Swapchain::Builder::RequestSurfaceFormat(const VkSurfaceFormatKHR format)
{
    m_SurfaceFormats.Insert(m_SurfaceFormats.begin(), format);
    return *this;
}
Swapchain::Builder &Swapchain::Builder::AllowSurfaceFormat(const VkSurfaceFormatKHR format)
{
    m_SurfaceFormats.Append(format);
    return *this;
}
Swapchain::Builder &Swapchain::Builder::RequestPresentMode(const VkPresentModeKHR mode)
{
    m_PresentModes.Insert(m_PresentModes.begin(), mode);
    return *this;
}
Swapchain::Builder &Swapchain::Builder::AllowPresentMode(const VkPresentModeKHR mode)
{
    m_PresentModes.Append(mode);
    return *this;
}
Swapchain::Builder &Swapchain::Builder::RequestImageCount(const u32 images)
{
    m_RequestedImages = images;
    if (m_RequestedImages < m_RequiredImages)
        m_RequiredImages = m_RequestedImages;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::RequireImageCount(const u32 images)
{
    m_RequiredImages = images;
    if (m_RequestedImages < m_RequiredImages)
        m_RequestedImages = m_RequiredImages;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::RequestExtent(const u32 width, const u32 height)
{
    m_Extent.width = width;
    m_Extent.height = height;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::RequestExtent(const VkExtent2D &extent)
{
    m_Extent = extent;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::SetFlags(const SwapchainBuilderFlags flags)
{
    m_Flags = flags;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::AddFlags(const SwapchainBuilderFlags flags)
{
    m_Flags |= flags;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::RemoveFlags(const SwapchainBuilderFlags flags)
{
    m_Flags &= ~flags;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::SetImageArrayLayers(const u32 layers)
{
    m_ImageArrayLayers = layers;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::SetCreateFlags(const VkSwapchainCreateFlagsKHR flags)
{
    m_CreateFlags = flags;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::AddCreateFlags(const VkSwapchainCreateFlagsKHR flags)
{
    m_CreateFlags |= flags;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::RemoveCreateFlags(const VkSwapchainCreateFlagsKHR flags)
{
    m_CreateFlags &= ~flags;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::SetImageUsageFlags(const VkImageUsageFlags flags)
{
    m_ImageUsage = flags;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::AddImageUsageFlags(const VkImageUsageFlags flags)
{
    m_ImageUsage |= flags;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::RemoveImageUsageFlags(const VkImageUsageFlags flags)
{
    m_ImageUsage &= ~flags;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::SetTransformBit(const VkSurfaceTransformFlagBitsKHR transform)
{
    m_TransformBit = transform;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::SetCompositeAlphaBit(const VkCompositeAlphaFlagBitsKHR alpha)
{
    m_CompositeAlphaFlags = alpha;
    return *this;
}
Swapchain::Builder &Swapchain::Builder::SetOldSwapchain(const VkSwapchainKHR oldSwapchain)
{
    m_OldSwapchain = oldSwapchain;
    return *this;
}

} // namespace VKit
#endif
