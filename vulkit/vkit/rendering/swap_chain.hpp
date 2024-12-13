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

namespace VKit
{
/**
 * @brief Represents a Vulkan swap chain and its associated resources.
 *
 * Manages the swap chain's images, depth resources, synchronization objects,
 * and frame buffers. Provides methods for creation, destruction, and resource management.
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
     * A VmaAllocator is only required if depth resources are created.
     */
    class Builder
    {
      public:
        /**
         * @brief Flags for configuring swap chain creation.
         *
         * These flags define optional behaviors for the swap chain, such as creating
         * default depth resources, sync objects, or image views.
         *
         * If sync objects are created, they will be destroyed along with the swapchain on resize even though
         * they could be kept, which may be undesirable.
         */
        enum FlagBits : u8
        {
            Flag_Clipped = 1 << 0,
            Flag_CreateImageViews = 1 << 1,
            Flag_CreateDefaultDepthResources = 1 << 2,
            Flag_CreateDefaultSyncObjects = 1 << 3
        };
        using Flags = u8;

        Builder(const LogicalDevice *p_Device, VkSurfaceKHR p_Surface) noexcept;

        /**
         * @brief Creates a swap chain based on the builder's configuration.
         *
         * Returns a swap chain object if the creation succeeds, or an error otherwise.
         *
         * @return A result containing the created SwapChain or an error.
         */
        Result<SwapChain> Build() const noexcept;

        Builder &SetAllocator(VmaAllocator p_Allocator) noexcept; // only required if depth resources are created

        Builder &RequestSurfaceFormat(VkSurfaceFormatKHR p_Format) noexcept;
        Builder &AllowSurfaceFormat(VkSurfaceFormatKHR p_Format) noexcept;

        Builder &RequestDepthFormat(VkFormat p_Format) noexcept;
        Builder &AllowDepthFormat(VkFormat p_Format) noexcept;

        Builder &RequestPresentMode(VkPresentModeKHR p_Mode) noexcept;
        Builder &AllowPresentMode(VkPresentModeKHR p_Mode) noexcept;

        Builder &RequestImageCount(u32 p_Images) noexcept;
        Builder &RequireImageCount(u32 p_Images) noexcept;

        Builder &RequestExtent(u32 p_Width, u32 p_Height) noexcept;
        Builder &RequestExtent(const VkExtent2D &p_Extent) noexcept;

        Builder &SetImageArrayLayers(u32 p_Layers) noexcept;

        Builder &SetFlags(Flags p_Flags) noexcept;
        Builder &AddFlags(Flags p_Flags) noexcept;
        Builder &RemoveFlags(Flags p_Flags) noexcept;

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

        Flags m_Flags = 0;
        VkSwapchainCreateFlagsKHR m_CreateFlags = 0;
        VkSurfaceTransformFlagBitsKHR m_TransformBit = static_cast<VkSurfaceTransformFlagBitsKHR>(0);
        VkCompositeAlphaFlagBitsKHR m_CompositeAlphaFlags = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    };

    /**
     * @brief Flags describing swap chain capabilities.
     *
     * Indicates features like whether the swap chain has image views, depth resources,
     * or default frame buffers.
     */
    enum FlagBits : u8
    {
        Flag_Clipped = 1 << 0,
        Flag_HasImageViews = 1 << 1,
        Flag_HasDefaultDepthResources = 1 << 2,
        Flag_HasDefaultSyncObjects = 1 << 3,
        Flag_HasDefaultFrameBuffers = 1 << 4
    };
    using Flags = u8;

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
        Flags Flags;

        TKit::StaticArray<ImageData, VKIT_MAX_IMAGE_COUNT> ImageData;
        std::array<SyncData, VKIT_MAX_FRAMES_IN_FLIGHT> SyncData;
    };

    SwapChain() noexcept = default;
    SwapChain(const LogicalDevice::Proxy &p_Device, VkSwapchainKHR p_SwapChain, const Info &p_Info) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    const Info &GetInfo() const noexcept;
    VkSwapchainKHR GetSwapChain() const noexcept;

    /**
     * @brief Creates default frame buffers for the swap chain's images.
     *
     * Associates the swap chain's images with the specified render pass and
     * creates frame buffers for each image. This simplifies rendering setup.
     *
     * The frame buffers are automatically destroyed when the swap chain is destroyed.
     *
     * @param p_RenderPass The render pass to associate the frame buffers with.
     * @return A VulkanResult indicating success or failure.
     */
    VulkanResult CreateDefaultFrameBuffers(VkRenderPass p_RenderPass) noexcept;

    explicit(false) operator VkSwapchainKHR() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit