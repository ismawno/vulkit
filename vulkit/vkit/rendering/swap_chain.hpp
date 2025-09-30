#pragma once

#ifndef VKIT_ENABLE_SWAP_CHAIN
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_SWAP_CHAIN"
#endif

#include "vkit/vulkan/logical_device.hpp"

namespace VKit
{
/**
 * @brief Represents a Vulkan swap chain and its associated resources.
 *
 * Manages the swap chain's images and, optionally, image views.
 */
class VKIT_API SwapChain
{
  public:
    /**
     * @brief Helps create and configure a Vulkan swap chain.
     *
     * Provides methods to specify swap chain parameters like surface format,
     * present mode, image count, and flags. Supports both mandatory and optional requirements.
     *
     */
    class Builder
    {
      public:
        using Flags = u8;
        /**
         * @brief Flags for configuring swap chain creation.
         *
         * These flags define optional behaviors for the swap chain, such as creating image views.
         *
         */
        enum FlagBits : Flags
        {
            Flag_None = 0,
            Flag_Clipped = 1 << 0,
            Flag_CreateImageViews = 1 << 1
        };

        Builder(const LogicalDevice *p_Device, VkSurfaceKHR p_Surface) : m_Device(p_Device), m_Surface(p_Surface)
        {
        }

        /**
         * @brief Creates a swap chain based on the builder's configuration.
         *
         * Returns a swap chain object if the creation succeeds, or an error otherwise.
         *
         * @return A `Result` containing the created `SwapChain` or an error.
         */
        Result<SwapChain> Build() const;

        Builder &RequestSurfaceFormat(VkSurfaceFormatKHR p_Format);
        Builder &AllowSurfaceFormat(VkSurfaceFormatKHR p_Format);

        Builder &RequestPresentMode(VkPresentModeKHR p_Mode);
        Builder &AllowPresentMode(VkPresentModeKHR p_Mode);

        Builder &RequestImageCount(u32 p_Images);
        Builder &RequireImageCount(u32 p_Images);

        Builder &RequestExtent(u32 p_Width, u32 p_Height);
        Builder &RequestExtent(const VkExtent2D &p_Extent);

        Builder &SetImageArrayLayers(u32 p_Layers);

        Builder &SetFlags(Flags p_Flags);
        Builder &AddFlags(Flags p_Flags);
        Builder &RemoveFlags(Flags p_Flags);

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

        TKit::StaticArray16<VkSurfaceFormatKHR> m_SurfaceFormats;
        TKit::StaticArray8<VkPresentModeKHR> m_PresentModes;

        VkImageUsageFlags m_ImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        Flags m_Flags = 0;
        VkSwapchainCreateFlagsKHR m_CreateFlags = 0;
        VkSurfaceTransformFlagBitsKHR m_TransformBit = static_cast<VkSurfaceTransformFlagBitsKHR>(0);
        VkCompositeAlphaFlagBitsKHR m_CompositeAlphaFlags = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    };

    using Flags = u8;
    /**
     * @brief Flags describing swap chain capabilities.
     *
     * Indicates features like whether the swap chain has image views.
     */
    enum FlagBits : Flags
    {
        Flag_None = 0,
        Flag_Clipped = 1 << 0,
        Flag_HasImageViews = 1 << 1
    };

    struct ImageData
    {
        VkImage Image;
        VkImageView ImageView = VK_NULL_HANDLE;
    };

    struct Info
    {
        VkSurfaceFormatKHR SurfaceFormat;
        VkPresentModeKHR PresentMode;

        VkExtent2D Extent;
        VkImageUsageFlags ImageUsage;

        Flags Flags;

        PhysicalDevice::SwapChainSupportDetails SupportDetails;
        TKit::StaticArray8<ImageData> ImageData;
    };

    SwapChain() = default;
    SwapChain(const LogicalDevice::Proxy &p_Device, VkSwapchainKHR p_SwapChain, const Info &p_Info)
        : m_Device(p_Device), m_SwapChain(p_SwapChain), m_Info(p_Info)
    {
    }

    void Destroy();
    void SubmitForDeletion(DeletionQueue &p_Queue) const;

    const LogicalDevice::Proxy &GetDevice() const
    {
        return m_Device;
    }
    VkSwapchainKHR GetHandle() const
    {
        return m_SwapChain;
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
    void destroy() const;

    LogicalDevice::Proxy m_Device{};
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
