#pragma once

#ifndef VKIT_ENABLE_SWAP_CHAIN
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_SWAP_CHAIN"
#endif

#include "vkit/device/logical_device.hpp"
#include "vkit/resource/image.hpp"

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
        Builder(const LogicalDevice *p_Device, VkSurfaceKHR p_Surface) : m_Device(p_Device), m_Surface(p_Surface)
        {
        }

        VKIT_NO_DISCARD Result<SwapChain> Build() const;

        Builder &RequestSurfaceFormat(VkSurfaceFormatKHR p_Format);
        Builder &AllowSurfaceFormat(VkSurfaceFormatKHR p_Format);

        Builder &RequestPresentMode(VkPresentModeKHR p_Mode);
        Builder &AllowPresentMode(VkPresentModeKHR p_Mode);

        Builder &RequestImageCount(u32 p_Images);
        Builder &RequireImageCount(u32 p_Images);

        Builder &RequestExtent(u32 p_Width, u32 p_Height);
        Builder &RequestExtent(const VkExtent2D &p_Extent);

        Builder &SetImageArrayLayers(u32 p_Layers);

        Builder &SetFlags(SwapChainBuilderFlags p_Flags);
        Builder &AddFlags(SwapChainBuilderFlags p_Flags);
        Builder &RemoveFlags(SwapChainBuilderFlags p_Flags);

        Builder &SetCreateFlags(VkSwapchainCreateFlagsKHR p_Flags);
        Builder &AddCreateFlags(VkSwapchainCreateFlagsKHR p_Flags);
        Builder &RemoveCreateFlags(VkSwapchainCreateFlagsKHR p_Flags);

        Builder &SetImageUsageFlags(VkImageUsageFlags p_Flags);
        Builder &AddImageUsageFlags(VkImageUsageFlags p_Flags);
        Builder &RemoveImageUsageFlags(VkImageUsageFlags p_Flags);

        Builder &SetTransformBit(VkSurfaceTransformFlagBitsKHR p_Transform);
        Builder &SetCompositeAlphaBit(VkCompositeAlphaFlagBitsKHR p_Alpha);

        Builder &SetOldSwapChain(VkSwapchainKHR p_OldSwapChain);

      private:
        const LogicalDevice *m_Device;
        VkSurfaceKHR m_Surface;

        VkSwapchainKHR m_OldSwapChain = VK_NULL_HANDLE;

        VkExtent2D m_Extent = {512, 512};

        u32 m_RequestedImages = 0;
        u32 m_RequiredImages = 0; // Zero means no requirement
        u32 m_ImageArrayLayers = 1;

        TKit::Array16<VkSurfaceFormatKHR> m_SurfaceFormats;
        TKit::Array8<VkPresentModeKHR> m_PresentModes;

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
    SwapChain(const ProxyDevice &p_Device, VkSwapchainKHR p_SwapChain, const TKit::Array8<DeviceImage> &p_Images,
              const Info &p_Info)
        : m_Device(p_Device), m_SwapChain(p_SwapChain), m_Images(p_Images), m_Info(p_Info)
    {
    }

    void Destroy();

    const ProxyDevice &GetDevice() const
    {
        return m_Device;
    }
    VkSwapchainKHR GetHandle() const
    {
        return m_SwapChain;
    }

    const DeviceImage &GetImage(const u32 p_Index) const
    {
        return m_Images[p_Index];
    }
    DeviceImage &GetImage(const u32 p_Index)
    {
        return m_Images[p_Index];
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
    TKit::Array8<DeviceImage> m_Images;
    Info m_Info;
};
} // namespace VKit
#endif
