#include "vkit/core/pch.hpp"
#include "vkit/backend/swap_chain.hpp"

namespace VKit
{
SwapChain::Builder::Builder(const LogicalDevice *p_Device, VkSurfaceKHR p_Surface) noexcept
    : m_Device(p_Device), m_Surface(p_Surface)
{
}

Result<VkSurfaceFormatKHR> selectFormat(const DynamicArray<VkSurfaceFormatKHR> &p_Requested,
                                        const DynamicArray<VkSurfaceFormatKHR> &p_Supported) noexcept
{
    for (const VkSurfaceFormatKHR &desired : p_Requested)
        for (const VkSurfaceFormatKHR &supported : p_Supported)
            if (desired.format == supported.format && desired.colorSpace == supported.colorSpace)
                return Result<VkSurfaceFormatKHR>::Ok(desired);
    return Result<VkSurfaceFormatKHR>::Error(VK_ERROR_FORMAT_NOT_SUPPORTED,
                                             "No desired format that is supported found");
}

Result<VkPresentModeKHR> selectPresentMode(const DynamicArray<VkPresentModeKHR> &p_Requested,
                                           const DynamicArray<VkPresentModeKHR> &p_Supported) noexcept
{
    for (const VkPresentModeKHR &desired : p_Requested)
        for (const VkPresentModeKHR &supported : p_Supported)
            if (desired == supported)
                return Result<VkPresentModeKHR>::Ok(desired);
    return Result<VkPresentModeKHR>::Error(VK_ERROR_FORMAT_NOT_SUPPORTED,
                                           "No desired present mode that is supported found");
}

Result<SwapChain> SwapChain::Builder::Build() const noexcept
{
    const PhysicalDevice::Info &devInfo = m_Device->GetPhysicalDevice().GetInfo();
    if (devInfo.GraphicsIndex == UINT32_MAX)
        return Result<SwapChain>::Error(VK_ERROR_INITIALIZATION_FAILED, "No graphics queue found");
    if (devInfo.PresentIndex == UINT32_MAX)
        return Result<SwapChain>::Error(VK_ERROR_INITIALIZATION_FAILED, "No present queue found");

    const auto checkFlag = [this](const SwapChainBuilderFlags p_Flag) -> bool { return m_Flags & p_Flag; };
    if (!m_Allocator && checkFlag(SwapChainBuilderFlags_CreateDefaultDepthResources))
        return Result<SwapChain>::Error(VK_ERROR_INITIALIZATION_FAILED,
                                        "If depth resources are created, an allocator must be provided");

    DynamicArray<VkSurfaceFormatKHR> imageFormats = m_SurfaceFormats;
    DynamicArray<VkFormat> depthFormats = m_DepthFormats;
    DynamicArray<VkPresentModeKHR> presentModes = m_PresentModes;
    if (imageFormats.empty())
        imageFormats.push_back({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});
    if (presentModes.empty())
    {
        presentModes.push_back(VK_PRESENT_MODE_MAILBOX_KHR);
        presentModes.push_back(VK_PRESENT_MODE_FIFO_KHR);
    }
    if (depthFormats.empty())
        depthFormats.push_back(VK_FORMAT_D32_SFLOAT);

    const auto suppResult = m_Device->QuerySwapChainSupport(m_Surface);
    if (!suppResult)
        return Result<SwapChain>::Error(suppResult.GetError());

    const PhysicalDevice::SwapChainSupportDetails &support = suppResult.GetValue();

    const auto checkImageCount = [&support](const u32 p_Count) -> const char * {
        if (p_Count < support.Capabilities.minImageCount)
            return "The requested image count is less than the minimum image count";
        if (support.Capabilities.maxImageCount > 0 && p_Count > support.Capabilities.maxImageCount)
            return "The requested image count is greater than the maximum image count";
        return nullptr;
    };

    u32 minImageCount = m_RequestedImages;
    if (checkImageCount(minImageCount))
        if (m_RequiredImages == 0)
        {
            minImageCount = support.Capabilities.minImageCount + 1;
            if (minImageCount > support.Capabilities.maxImageCount)
                minImageCount = support.Capabilities.maxImageCount;
        }
        else
        {
            minImageCount = m_RequiredImages;
            const char *error = checkImageCount(minImageCount);
            if (error)
                return Result<SwapChain>::Error(VK_ERROR_INITIALIZATION_FAILED, error);
        }
    TKIT_ASSERT(minImageCount <= VKIT_MAX_IMAGE_COUNT, "The image count is too high");

    const auto surfFormatResult = selectFormat(imageFormats, support.Formats);
    if (!surfFormatResult)
        return Result<SwapChain>::Error(surfFormatResult.GetError());

    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    if (checkFlag(SwapChainBuilderFlags_CreateDefaultDepthResources))
    {
        const auto depFormatResult = m_Device->FindSupportedFormat(depthFormats, VK_IMAGE_TILING_OPTIMAL,
                                                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        if (!depFormatResult)
            return Result<SwapChain>::Error(depFormatResult.GetError());
        depthFormat = depFormatResult.GetValue();
    }

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
        extent.width =
            std::clamp(m_Width, support.Capabilities.minImageExtent.width, support.Capabilities.maxImageExtent.width);
        extent.height = std::clamp(m_Height, support.Capabilities.minImageExtent.height,
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
    createInfo.clipped = checkFlag(SwapChainBuilderFlags_Clipped);
    createInfo.oldSwapchain = m_OldSwapChain;
    createInfo.flags = m_CreateFlags;

    const auto createSwapChain = m_Device->GetFunction<PFN_vkCreateSwapchainKHR>("vkCreateSwapchainKHR");
    if (!createSwapChain)
        return Result<SwapChain>::Error(VK_ERROR_INITIALIZATION_FAILED,
                                        "Failed to get the vkCreateSwapchainKHR function");

    const auto destroySwapChain = m_Device->GetFunction<PFN_vkDestroySwapchainKHR>("vkDestroySwapchainKHR");
    if (!destroySwapChain)
        return Result<SwapChain>::Error(VK_ERROR_INITIALIZATION_FAILED,
                                        "Failed to get the vkDestroySwapchainKHR function");

    const LogicalDevice::Proxy proxy = m_Device->CreateProxy();
    VkSwapchainKHR swapChain;
    VkResult result = createSwapChain(proxy, &createInfo, proxy.AllocationCallbacks, &swapChain);
    if (result != VK_SUCCESS)
        return Result<SwapChain>::Error(result, "Failed to create the swap chain");

    const auto getImages = m_Device->GetFunction<PFN_vkGetSwapchainImagesKHR>("vkGetSwapchainImagesKHR");
    if (!getImages)
    {
        destroySwapChain(proxy, swapChain, proxy.AllocationCallbacks);
        return Result<SwapChain>::Error(VK_ERROR_INITIALIZATION_FAILED,
                                        "Failed to get the vkGetSwapchainImagesKHR function");
    }
    SwapChain::Info info{};
    info.Extent = extent;
    info.SurfaceFormat = surfaceFormat;
    info.DepthFormat = depthFormat;
    info.PresentMode = presentMode;
    info.ImageUsage = m_ImageUsage;
    info.Allocator = m_Allocator;
    info.Flags = m_Flags;
    info.ImageUsage = m_ImageUsage;

    u32 imageCount;
    result = getImages(proxy, swapChain, &imageCount, nullptr);
    if (result != VK_SUCCESS)
    {
        destroySwapChain(proxy, swapChain, proxy.AllocationCallbacks);
        return Result<SwapChain>::Error(result, "Failed to get the swap chain images count");
    }

    TKit::StaticArray<VkImage, VKIT_MAX_IMAGE_COUNT> images;
    images.resize(imageCount);

    result = getImages(proxy, swapChain, &imageCount, images.data());
    if (result != VK_SUCCESS)
    {
        destroySwapChain(proxy, swapChain, proxy.AllocationCallbacks);
        return Result<SwapChain>::Error(result, "Failed to get the swap chain images");
    }

    info.ImageData.resize(imageCount);
    for (u32 i = 0; i < imageCount; ++i)
    {
        info.ImageData[i].Image = images[i];
        if (checkFlag(SwapChainBuilderFlags_CreateImageViews))
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

            result = vkCreateImageView(proxy, &viewInfo, proxy.AllocationCallbacks, &info.ImageData[i].ImageView);
            if (result != VK_SUCCESS)
            {
                for (u32 j = 0; j < i; ++j)
                    vkDestroyImageView(proxy, info.ImageData[j].ImageView, proxy.AllocationCallbacks);
                destroySwapChain(proxy, swapChain, proxy.AllocationCallbacks);
                return Result<SwapChain>::Error(result, "Failed to create the image view");
            }
        }

        if (checkFlag(SwapChainBuilderFlags_CreateDefaultDepthResources))
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = extent.width;
            imageInfo.extent.height = extent.height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = depthFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.flags = 0;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            result = vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &info.ImageData[i].DepthImage,
                                    &info.ImageData[i].DepthAllocation, nullptr);
            if (result != VK_SUCCESS)
            {
                if (checkFlag(SwapChainBuilderFlags_CreateImageViews))
                    for (u32 j = 0; j <= i; ++j)
                        vkDestroyImageView(proxy, info.ImageData[j].ImageView, proxy.AllocationCallbacks);
                for (u32 j = 0; j < i; ++j)
                    vmaDestroyImage(m_Allocator, info.ImageData[j].DepthImage, info.ImageData[j].DepthAllocation);

                destroySwapChain(proxy, swapChain, proxy.AllocationCallbacks);
                return Result<SwapChain>::Error(result, "Failed to create the depth image");
            }

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = info.ImageData[i].DepthImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = depthFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            result = vkCreateImageView(proxy, &viewInfo, proxy.AllocationCallbacks, &info.ImageData[i].DepthImageView);
            if (result != VK_SUCCESS)
            {
                for (u32 j = 0; j <= i; ++j)
                {
                    if (checkFlag(SwapChainBuilderFlags_CreateImageViews))
                        vkDestroyImageView(proxy, info.ImageData[j].ImageView, proxy.AllocationCallbacks);
                    vmaDestroyImage(m_Allocator, info.ImageData[j].DepthImage, info.ImageData[j].DepthAllocation);
                }
                for (u32 j = 0; j < i; ++j)
                    vkDestroyImageView(proxy, info.ImageData[j].DepthImageView, proxy.AllocationCallbacks);

                destroySwapChain(proxy, swapChain, proxy.AllocationCallbacks);
                return Result<SwapChain>::Error(result, "Failed to create the depth image view");
            }
        }
    }

    if (checkFlag(SwapChainBuilderFlags_CreateDefaultSyncObjects))
        for (u32 i = 0; i < VKIT_MAX_FRAMES_IN_FLIGHT; ++i)
        {
            const auto cleanup = [&](const u32 p_Index) {
                if (checkFlag(SwapChainBuilderFlags_CreateImageViews))
                    for (u32 j = 0; j <= p_Index; ++j)
                        vkDestroyImageView(proxy, info.ImageData[j].ImageView, proxy.AllocationCallbacks);
                if (checkFlag(SwapChainBuilderFlags_CreateDefaultDepthResources))
                    for (u32 j = 0; j <= p_Index; ++j)
                    {
                        vmaDestroyImage(m_Allocator, info.ImageData[j].DepthImage, info.ImageData[j].DepthAllocation);
                        vkDestroyImageView(proxy, info.ImageData[j].DepthImageView, proxy.AllocationCallbacks);
                    }

                destroySwapChain(proxy, swapChain, proxy.AllocationCallbacks);
            };

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            result = vkCreateSemaphore(proxy, &semaphoreInfo, proxy.AllocationCallbacks,
                                       &info.SyncData[i].ImageAvailableSemaphore);
            if (result != VK_SUCCESS)
            {
                for (u32 j = 0; j < i; ++j)
                {
                    vkDestroySemaphore(proxy, info.SyncData[j].ImageAvailableSemaphore, proxy.AllocationCallbacks);
                    vkDestroySemaphore(proxy, info.SyncData[j].RenderFinishedSemaphore, proxy.AllocationCallbacks);
                    vkDestroyFence(proxy, info.SyncData[j].InFlightFence, proxy.AllocationCallbacks);
                }
                cleanup(i);
                return Result<SwapChain>::Error(result, "Failed to create the image available semaphore");
            }

            result = vkCreateSemaphore(proxy, &semaphoreInfo, proxy.AllocationCallbacks,
                                       &info.SyncData[i].RenderFinishedSemaphore);
            if (result != VK_SUCCESS)
            {
                for (u32 j = 0; j <= i; ++j)
                    vkDestroySemaphore(proxy, info.SyncData[j].ImageAvailableSemaphore, proxy.AllocationCallbacks);
                for (u32 j = 0; j < i; ++j)
                {
                    vkDestroySemaphore(proxy, info.SyncData[j].RenderFinishedSemaphore, proxy.AllocationCallbacks);
                    vkDestroyFence(proxy, info.SyncData[j].InFlightFence, proxy.AllocationCallbacks);
                }
                cleanup(i);
                return Result<SwapChain>::Error(result, "Failed to create the image available semaphore");
            }

            result = vkCreateFence(proxy, &fenceInfo, proxy.AllocationCallbacks, &info.SyncData[i].InFlightFence);
            if (result != VK_SUCCESS)
            {
                for (u32 j = 0; j <= i; ++j)
                {
                    vkDestroySemaphore(proxy, info.SyncData[j].ImageAvailableSemaphore, proxy.AllocationCallbacks);
                    vkDestroySemaphore(proxy, info.SyncData[j].RenderFinishedSemaphore, proxy.AllocationCallbacks);
                }
                for (u32 j = 0; j < i; ++j)
                    vkDestroyFence(proxy, info.SyncData[j].InFlightFence, proxy.AllocationCallbacks);
                cleanup(i);
                return Result<SwapChain>::Error(result, "Failed to create the image available semaphore");
            }
        }
    return Result<SwapChain>::Ok(proxy, swapChain, info);
}

VulkanResult SwapChain::CreateDefaultFrameBuffers(const VkRenderPass p_RenderPass) noexcept
{
    for (u32 i = 0; i < m_Info.ImageData.size(); ++i)
    {
        std::array<VkImageView, 2> attachments = {m_Info.ImageData[i].ImageView, m_Info.ImageData[i].DepthImageView};
        const u32 attachmentCount = attachments[1] ? 2 : 1;

        VkFramebufferCreateInfo frameBufferInfo{};
        frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferInfo.renderPass = p_RenderPass;
        frameBufferInfo.attachmentCount = attachmentCount;
        frameBufferInfo.pAttachments = attachments.data();
        frameBufferInfo.width = m_Info.Extent.width;
        frameBufferInfo.height = m_Info.Extent.height;
        frameBufferInfo.layers = 1;

        const VkResult result = vkCreateFramebuffer(m_Device, &frameBufferInfo, m_Device.AllocationCallbacks,
                                                    &m_Info.ImageData[i].FrameBuffer);
        if (result != VK_SUCCESS)
        {
            for (u32 j = 0; j < i; ++j)
                vkDestroyFramebuffer(m_Device, m_Info.ImageData[j].FrameBuffer, m_Device.AllocationCallbacks);
            return VulkanResult::Error(result, "Failed to create the frame buffer");
        }
    }
    m_Info.Flags |= SwapChainFlags_HasDefaultFrameBuffers;
    return VulkanResult::Success();
}

void SwapChain::Destroy() noexcept
{
    TKIT_ASSERT(m_SwapChain, "The swap chain is already destroyed");

    const auto destroySwapChain =
        System::GetDeviceFunction<PFN_vkDestroySwapchainKHR>("vkDestroySwapchainKHR", m_Device);
    TKIT_ASSERT(destroySwapChain, "Failed to get the vkDestroySwapchainKHR function");

    if (m_Info.Flags & SwapChainFlags_HasImageViews)
        for (const ImageData &data : m_Info.ImageData)
            vkDestroyImageView(m_Device, data.ImageView, m_Device.AllocationCallbacks);

    destroySwapChain(m_Device, m_SwapChain, m_Device.AllocationCallbacks);

    if (m_Info.Flags & SwapChainFlags_HasDefaultDepthResources)
        for (const ImageData &data : m_Info.ImageData)
        {
            vkDestroyImageView(m_Device, data.DepthImageView, m_Device.AllocationCallbacks);
            vmaDestroyImage(m_Info.Allocator, data.DepthImage, data.DepthAllocation);
        }

    if (m_Info.Flags & SwapChainFlags_HasDefaultFrameBuffers)
        for (const ImageData &data : m_Info.ImageData)
            vkDestroyFramebuffer(m_Device, data.FrameBuffer, m_Device.AllocationCallbacks);

    if (m_Info.Flags & SwapChainFlags_HasDefaultSyncObjects)
        for (SyncData &data : m_Info.SyncData)
        {
            vkDestroySemaphore(m_Device, data.RenderFinishedSemaphore, m_Device.AllocationCallbacks);
            vkDestroySemaphore(m_Device, data.ImageAvailableSemaphore, m_Device.AllocationCallbacks);
            vkDestroyFence(m_Device, data.InFlightFence, m_Device.AllocationCallbacks);
        }
    m_SwapChain = VK_NULL_HANDLE;
}
void SwapChain::SubmitForDeletion(DeletionQueue &p_Queue) noexcept
{
    const SwapChain swapChain = *this;
    p_Queue.Push([swapChain]() { SwapChain{swapChain}.Destroy(); }); // That is stupid...
}

VkSwapchainKHR SwapChain::GetSwapChain() const noexcept
{
    return m_SwapChain;
}
const SwapChain::Info &SwapChain::GetInfo() const noexcept
{
    return m_Info;
}
SwapChain::operator VkSwapchainKHR() const noexcept
{
    return m_SwapChain;
}
SwapChain::operator bool() const noexcept
{
    return m_SwapChain != VK_NULL_HANDLE;
}

SwapChain::Builder &SwapChain::Builder::RequestSurfaceFormat(const VkSurfaceFormatKHR p_Format) noexcept
{
    m_SurfaceFormats.insert(m_SurfaceFormats.begin(), p_Format);
    return *this;
}
SwapChain::Builder &SwapChain::Builder::AllowSurfaceFormat(const VkSurfaceFormatKHR p_Format) noexcept
{
    m_SurfaceFormats.push_back(p_Format);
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RequestPresentMode(const VkPresentModeKHR p_Mode) noexcept
{
    m_PresentModes.insert(m_PresentModes.begin(), p_Mode);
    return *this;
}
SwapChain::Builder &SwapChain::Builder::AllowPresentMode(const VkPresentModeKHR p_Mode) noexcept
{
    m_PresentModes.push_back(p_Mode);
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RequestImageCount(const u32 p_Images) noexcept
{
    m_RequestedImages = p_Images;
    if (m_RequestedImages < m_RequiredImages)
        m_RequiredImages = m_RequestedImages;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RequireImageCount(const u32 p_Images) noexcept
{
    m_RequiredImages = p_Images;
    if (m_RequestedImages < m_RequiredImages)
        m_RequestedImages = m_RequiredImages;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RequestExtent(const u32 p_Width, const u32 p_Height) noexcept
{
    m_Width = p_Width;
    m_Height = p_Height;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetAllocator(VmaAllocator p_Allocator) noexcept
{
    m_Allocator = p_Allocator;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetFlags(const u8 p_Flags) noexcept
{
    m_Flags = p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::AddFlags(const u8 p_Flags) noexcept
{
    m_Flags |= p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RemoveFlags(const u8 p_Flags) noexcept
{
    m_Flags &= ~p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetImageArrayLayers(const u32 p_Layers) noexcept
{
    m_ImageArrayLayers = p_Layers;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetCreateFlags(const VkSwapchainCreateFlagsKHR p_Flags) noexcept
{
    m_CreateFlags = p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::AddCreateFlags(const VkSwapchainCreateFlagsKHR p_Flags) noexcept
{
    m_CreateFlags |= p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RemoveCreateFlags(const VkSwapchainCreateFlagsKHR p_Flags) noexcept
{
    m_CreateFlags &= ~p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetImageUsageFlags(const VkImageUsageFlags p_Flags) noexcept
{
    m_ImageUsage = p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::AddImageUsageFlags(const VkImageUsageFlags p_Flags) noexcept
{
    m_ImageUsage |= p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::RemoveImageUsageFlags(const VkImageUsageFlags p_Flags) noexcept
{
    m_ImageUsage &= ~p_Flags;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetTransformBit(const VkSurfaceTransformFlagBitsKHR p_Transform) noexcept
{
    m_TransformBit = p_Transform;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetCompositeAlphaBit(const VkCompositeAlphaFlagBitsKHR p_Alpha) noexcept
{
    m_CompositeAlphaFlags = p_Alpha;
    return *this;
}
SwapChain::Builder &SwapChain::Builder::SetOldSwapChain(const VkSwapchainKHR p_OldSwapChain) noexcept
{
    m_OldSwapChain = p_OldSwapChain;
    return *this;
}

} // namespace VKit