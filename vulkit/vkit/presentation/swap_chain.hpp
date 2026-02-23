#pragma once

#ifndef VKIT_ENABLE_SWAP_CHAIN
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_SWAP_CHAIN"
#endif

#include "vkit/device/logical_device.hpp"
#include "vkit/resource/device_image.hpp"

#ifndef VK_KHR_surface
#    error "[VULKIT] To use the swap chain abstraction, the vulkan headers must support the surface extension"
#else

namespace VKit
{
using SwapChainBuilderFlags = u8;
enum SwapChainBuilderFlagBits : SwapChainBuilderFlags
{
    SwapChainBuilderFlag_Clipped = 1 << 0,
    SwapChainBuilderFlag_CreateImageViews = 1 << 1
};

using SwapChainFlags = u8;
enum SwapChainFlagBits : SwapChainFlags
{
    SwapChainFlag_Clipped = 1 << 0,
    SwapChainFlag_HasImageViews = 1 << 1
};
class SwapChain
{
  public:
    class Builder
    {
      public:
        Builder(const LogicalDevice *device, VkSurfaceKHR surface) : m_Device(device), m_Surface(surface)
        {
        }

        VKIT_NO_DISCARD Result<SwapChain> Build() const;

        Builder &RequestSurfaceFormat(VkSurfaceFormatKHR format);
        Builder &AllowSurfaceFormat(VkSurfaceFormatKHR format);

        Builder &RequestPresentMode(VkPresentModeKHR mode);
        Builder &AllowPresentMode(VkPresentModeKHR mode);

        Builder &RequestImageCount(u32 images);
        Builder &RequireImageCount(u32 images);

        Builder &RequestExtent(u32 width, u32 height);
        Builder &RequestExtent(const VkExtent2D &extent);

        Builder &SetImageArrayLayers(u32 layers);

        Builder &SetFlags(SwapChainBuilderFlags flags);
        Builder &AddFlags(SwapChainBuilderFlags flags);
        Builder &RemoveFlags(SwapChainBuilderFlags flags);

        Builder &SetCreateFlags(VkSwapchainCreateFlagsKHR flags);
        Builder &AddCreateFlags(VkSwapchainCreateFlagsKHR flags);
        Builder &RemoveCreateFlags(VkSwapchainCreateFlagsKHR flags);

        Builder &SetImageUsageFlags(VkImageUsageFlags flags);
        Builder &AddImageUsageFlags(VkImageUsageFlags flags);
        Builder &RemoveImageUsageFlags(VkImageUsageFlags flags);

        Builder &SetTransformBit(VkSurfaceTransformFlagBitsKHR transform);
        Builder &SetCompositeAlphaBit(VkCompositeAlphaFlagBitsKHR alpha);

        Builder &SetOldSwapChain(VkSwapchainKHR oldSwapChain);

      private:
        const LogicalDevice *m_Device;
        VkSurfaceKHR m_Surface;

        VkSwapchainKHR m_OldSwapChain = VK_NULL_HANDLE;

        VkExtent2D m_Extent = {512, 512};

        u32 m_RequestedImages = 0;
        u32 m_RequiredImages = 0; // Zero means no requirement
        u32 m_ImageArrayLayers = 1;

        TKit::TierArray<VkSurfaceFormatKHR> m_SurfaceFormats;
        TKit::TierArray<VkPresentModeKHR> m_PresentModes;

        VkImageUsageFlags m_ImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        SwapChainBuilderFlags m_Flags = 0;
        VkSwapchainCreateFlagsKHR m_CreateFlags = 0;
        VkSurfaceTransformFlagBitsKHR m_TransformBit = static_cast<VkSurfaceTransformFlagBitsKHR>(0);
        VkCompositeAlphaFlagBitsKHR m_CompositeAlphaFlags = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    };

    struct Info
    {
        VkSurfaceFormatKHR SurfaceFormat;
        VkPresentModeKHR PresentMode;

        VkExtent2D Extent;
        VkImageUsageFlags ImageUsage;

        PhysicalDevice::SwapChainSupportDetails SupportDetails;

        SwapChainFlags Flags;
    };

    SwapChain() = default;
    SwapChain(const ProxyDevice &device, VkSwapchainKHR swapChain, const TKit::TierArray<DeviceImage> &images,
              const Info &info)
        : m_Device(device), m_SwapChain(swapChain), m_Images(images), m_Info(info)
    {
    }

    void Destroy();

    VKIT_SET_DEBUG_NAME(m_SwapChain, VK_OBJECT_TYPE_SWAPCHAIN_KHR)

    const ProxyDevice &GetDevice() const
    {
        return m_Device;
    }
    VkSwapchainKHR GetHandle() const
    {
        return m_SwapChain;
    }

    const DeviceImage &GetImage(const u32 index) const
    {
        return m_Images[index];
    }
    DeviceImage &GetImage(const u32 index)
    {
        return m_Images[index];
    }
    u32 GetImageCount() const
    {
        return m_Images.GetSize();
    }

    const Info &GetInfo() const
    {
        return m_Info;
    }
    operator VkSwapchainKHR() const
    {
        return m_SwapChain;
    }
    operator bool() const
    {
        return m_SwapChain != VK_NULL_HANDLE;
    }

  private:
    ProxyDevice m_Device{};
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    TKit::TierArray<DeviceImage> m_Images;
    Info m_Info;
};
} // namespace VKit
#endif
