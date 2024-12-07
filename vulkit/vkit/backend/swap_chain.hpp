#pragma once

#include "vkit/core/alias.hpp"
#include "vkit/backend/logical_device.hpp"
#include "tkit/container/static_array.hpp"

#ifndef VKIT_MAX_IMAGE_COUNT
#    define VKIT_MAX_IMAGE_COUNT 3
#endif

#ifndef VKIT_MAX_FRAMES_IN_FLIGHT
#    define VKIT_MAX_FRAMES_IN_FLIGHT 2
#endif

// Warning!: If sync objects are created, they will be destroyed along with the swapchain on resize even though
// they could be kept, which may (or may not) be undesirable.

namespace VKit
{
enum SwapChainBuilderFlags : u8
{
    SwapChainBuilderFlags_Clipped = 1 << 0,
    SwapChainBuilderFlags_CreateImageViews = 1 << 1,
    SwapChainBuilderFlags_CreateDefaultDepthResources = 1 << 2,
    SwapChainBuilderFlags_CreateDefaultSyncObjects = 1 << 3
};

enum SwapChainFlags : u8
{
    SwapChainFlags_Clipped = 1 << 0,
    SwapChainFlags_HasImageViews = 1 << 1,
    SwapChainFlags_HasDefaultDepthResources = 1 << 2,
    SwapChainFlags_HasDefaultSyncObjects = 1 << 3,
    SwapChainFlags_HasDefaultFrameBuffers = 1 << 4
};

class SwapChain
{
  public:
    struct ImageData
    {
        VkImage Image;
        VkImageView ImageView = VK_NULL_HANDLE;

        VkImage DepthImage = VK_NULL_HANDLE;
        VkImageView DepthImageView = VK_NULL_HANDLE;
        VmaAllocation DepthAllocation = VK_NULL_HANDLE;

        VkFramebuffer FrameBuffer = VK_NULL_HANDLE;
    };

    struct SyncData
    {
        VkSemaphore ImageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence InFlightFence = VK_NULL_HANDLE;
    };

    struct Info
    {
        VkSurfaceFormatKHR SurfaceFormat;
        VkFormat DepthFormat; // Optional, undefined if not created
        VmaAllocator Allocator;

        VkPresentModeKHR PresentMode;
        VkExtent2D Extent;
        VkImageUsageFlags ImageUsage;
        u8 Flags;

        TKit::StaticArray<ImageData, VKIT_MAX_IMAGE_COUNT> ImageData;
        std::array<SyncData, VKIT_MAX_FRAMES_IN_FLIGHT> SyncData;
    };

    class Builder
    {
      public:
        Builder(const LogicalDevice *p_Device, VkSurfaceKHR p_Surface) noexcept;

        Result<SwapChain> Build() const noexcept;

        Builder &SetAllocator(VmaAllocator p_Allocator) noexcept; // only required if depth resources are created

        Builder &RequestSurfaceFormat(VkSurfaceFormatKHR p_Format) noexcept;
        Builder &AllowSurfaceFormat(VkSurfaceFormatKHR p_Format) noexcept;

        Builder &RequestPresentMode(VkPresentModeKHR p_Mode) noexcept;
        Builder &AllowPresentMode(VkPresentModeKHR p_Mode) noexcept;

        Builder &RequestImageCount(u32 p_Images) noexcept;
        Builder &RequireImageCount(u32 p_Images) noexcept;

        Builder &RequestExtent(u32 p_Width, u32 p_Height) noexcept;
        Builder &RequestExtent(const VkExtent2D &p_Extent) noexcept;

        Builder &SetImageArrayLayers(u32 p_Layers) noexcept;

        Builder &SetFlags(u8 p_Flags) noexcept;
        Builder &AddFlags(u8 p_Flags) noexcept;
        Builder &RemoveFlags(u8 p_Flags) noexcept;

        Builder &SetCreateFlags(VkSwapchainCreateFlagsKHR p_Flags) noexcept;
        Builder &AddCreateFlags(VkSwapchainCreateFlagsKHR p_Flags) noexcept;
        Builder &RemoveCreateFlags(VkSwapchainCreateFlagsKHR p_Flags) noexcept;

        Builder &SetImageUsageFlags(VkImageUsageFlags p_Flags) noexcept;
        Builder &AddImageUsageFlags(VkImageUsageFlags p_Flags) noexcept;
        Builder &RemoveImageUsageFlags(VkImageUsageFlags p_Flags) noexcept;

        Builder &SetTransformBit(VkSurfaceTransformFlagBitsKHR p_Transform) noexcept;
        Builder &SetCompositeAlphaBit(VkCompositeAlphaFlagBitsKHR p_Alpha) noexcept;

        Builder &SetOldSwapChain(VkSwapchainKHR p_OldSwapChain) noexcept;

      private:
        const LogicalDevice *m_Device;
        VkSurfaceKHR m_Surface;

        VkSwapchainKHR m_OldSwapChain = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;

        u32 m_Width = 512;
        u32 m_Height = 512;

        u32 m_RequestedImages = 0;
        u32 m_RequiredImages = 0; // Zero means no requirement
        u32 m_ImageArrayLayers = 1;

        DynamicArray<VkSurfaceFormatKHR> m_SurfaceFormats;
        DynamicArray<VkFormat> m_DepthFormats;
        DynamicArray<VkPresentModeKHR> m_PresentModes;

        VkImageUsageFlags m_ImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        u8 m_Flags = 0;
        VkSwapchainCreateFlagsKHR m_CreateFlags = 0;
        VkSurfaceTransformFlagBitsKHR m_TransformBit = static_cast<VkSurfaceTransformFlagBitsKHR>(0);
        VkCompositeAlphaFlagBitsKHR m_CompositeAlphaFlags = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    };

    SwapChain() noexcept = default;
    SwapChain(const LogicalDevice::Proxy &p_Device, VkSwapchainKHR p_SwapChain, const Info &p_Info) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    const Info &GetInfo() const noexcept;
    VkSwapchainKHR GetSwapChain() const noexcept;

    // User does not need to destroy the frame buffers!
    VulkanResult CreateDefaultFrameBuffers(VkRenderPass p_RenderPass) noexcept;

    explicit(false) operator VkSwapchainKHR() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit