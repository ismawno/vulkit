#pragma once

#include "vkit/core/alias.hpp"
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
        enum FlagBit : Flags
        {
            Flag_Clipped = 1 << 0,
            Flag_CreateImageViews = 1 << 1
        };

        Builder(const LogicalDevice *p_Device, VkSurfaceKHR p_Surface) noexcept;

        /**
         * @brief Creates a swap chain based on the builder's configuration.
         *
         * Returns a swap chain object if the creation succeeds, or an error otherwise.
         *
         * @return A `Result` containing the created `SwapChain` or an error.
         */
        Result<SwapChain> Build() const noexcept;

        Builder &RequestSurfaceFormat(VkSurfaceFormatKHR p_Format) noexcept;
        Builder &AllowSurfaceFormat(VkSurfaceFormatKHR p_Format) noexcept;

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
    enum FlagBit : Flags
    {
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
        TKit::StaticArray4<ImageData> ImageData;
    };

    SwapChain() noexcept = default;
    SwapChain(const LogicalDevice::Proxy &p_Device, VkSwapchainKHR p_SwapChain, const Info &p_Info) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    const Info &GetInfo() const noexcept;
    VkSwapchainKHR GetSwapChain() const noexcept;

    explicit(false) operator VkSwapchainKHR() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    void destroy() const noexcept;

    LogicalDevice::Proxy m_Device{};
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    Info m_Info;
};

struct SyncData
{
    VkSemaphore ImageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence InFlightFence = VK_NULL_HANDLE;
};

/**
 * @brief Creates synchronization objects for the swap chain.
 *
 * Initializes semaphores and fences required for synchronization during rendering.
 * Each `SyncData` structure in the provided span must initially have its members set to `VK_NULL_HANDLE`.
 *
 * @param p_Device The logical device to create the synchronization objects with.
 * @param p_Objects A span of `SyncData` structures to populate with the created objects.
 * @return A VulkanResult indicating success or failure of the operation.
 */
VulkanResult CreateSynchronizationObjects(const LogicalDevice::Proxy &p_Device,
                                          TKit::Span<SyncData> p_Objects) noexcept;

/**
 * @brief Destroys synchronization objects.
 *
 * Releases Vulkan resources associated with the semaphores and fences in the given span of `SyncData` structures.
 *
 * @param p_Device The logical device used to destroy the synchronization objects.
 * @param p_Objects A span of `SyncData` structures whose resources will be destroyed.
 */
void DestroySynchronizationObjects(const LogicalDevice::Proxy &p_Device, TKit::Span<const SyncData> p_Objects) noexcept;

} // namespace VKit