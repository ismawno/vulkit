#pragma once

#ifndef VKIT_ENABLE_IMAGES
#    error "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_IMAGES"
#endif

#include "vkit/vulkan/logical_device.hpp"
#include "vkit/vulkan/allocator.hpp"

namespace VKit
{
using AttachmentFlags = u8;
enum AttachmentFlagBits
{
    AttachmentFlag_None = 0,
    AttachmentFlag_Color = 1 << 0,
    AttachmentFlag_Depth = 1 << 1,
    AttachmentFlag_Stencil = 1 << 2,
    AttachmentFlag_Input = 1 << 3,
    AttachmentFlag_Sampled = 1 << 4,
};

struct Image
{
    VmaAllocation Allocation;
    VkImage Image;
    VkImageView ImageView;
};

/**
 * @brief A nice `ImageHouse` that manages construction and destruction of images.
 *
 * Images created with an `ImageHouse` must be destroyed manually with the same `ImageHouse` they were created with.
 *
 */
class ImageHouse
{
  public:
    ImageHouse() noexcept = default;
    ImageHouse(const LogicalDevice::Proxy &p_Device, VmaAllocator p_Allocator) noexcept;

    static Result<ImageHouse> Create(const LogicalDevice::Proxy &p_Device, VmaAllocator p_Allocator) noexcept;

    /**
     * @brief Creates an image.
     *
     * Generates Vulkan image handles, views, and allocations suitable for an attachment, based on the provided image
     * configuration.
     *
     * @param p_Info The Vulkan image creation info.
     * @param p_Range The subresource range for the image.
     * @param p_ViewType The type of image view to create.
     * @return A `Result` containing the created `Image` or an error.
     */
    Result<Image> CreateImage(const VkImageCreateInfo &p_Info, const VkImageSubresourceRange &p_Range,
                              VkImageViewType p_ViewType) const noexcept;

    /**
     * @brief Creates an image.
     *
     * Generates Vulkan image handles, views, and allocations suitable for an attachment, based on the provided image
     * configuration.
     *
     * The view type is determined based on the image type in the image creation info.
     *
     * @param p_Info The Vulkan image creation info.
     * @param p_Range The subresource range for the image.
     * @return A `Result` containing the created `Image` or an error.
     */
    Result<Image> CreateImage(const VkImageCreateInfo &p_Info, const VkImageSubresourceRange &p_Range) const noexcept;

    /**
     * @brief Creates an image.
     *
     * Generates Vulkan image handles, views, and allocations suitable for an attachment, based on the provided image
     * configuration.
     *
     * The subresource range will default to the entire image.
     *
     * @param p_Info The Vulkan image creation info.
     * @param p_ViewType The type of image view to create.
     * @param p_Flags The flags of the attachment this image will be used for.
     * @return A `Result` containing the created `Image` or an error.
     */
    Result<Image> CreateImage(const VkImageCreateInfo &p_Info, VkImageViewType p_ViewType,
                              AttachmentFlags p_Flags) const noexcept;

    /**
     * @brief Creates an image.
     *
     * Generates Vulkan image handles, views, and allocations suitable for an attachment, based on the provided image
     * configuration.
     *
     * The view type is determined based on the image type in the image creation info.
     * The subresource range will default to the entire image.
     *
     * @param p_Info The Vulkan image creation info.
     * @param p_Flags The flags of the attachment this image will be used for.
     * @return A `Result` containing the created `Image` or an error.
     */
    Result<Image> CreateImage(const VkImageCreateInfo &p_Info, AttachmentFlags p_Flags) const noexcept;

    /**
     * @brief Creates an image.
     *
     * Generates Vulkan image handles, views, and allocations suitable for an attachment, based on the provided image
     * configuration.
     *
     * The image creation info will default to the attachment's format and usage flags to provide a
     * basic image resource that works for most cases.
     *
     * @param p_Extent The extent of the image.
     * @param p_Flags The flags of the attachment this image will be used for.
     * @return A `Result` containing the created `Image` or an error.
     */
    Result<Image> CreateImage(VkFormat p_Format, const VkExtent2D &p_Extent, AttachmentFlags p_Flags) const noexcept;

    /**
     * @brief Creates an image.
     *
     * A symbolic method where the user provides the handles for the images, used for example when the images are
     * obtained through a swap chain.
     *
     * An image created this way does not need to be destroyed with the `ImageHouse`, and will be ignored if passed into
     * `DestroyImage`.
     *
     * @param p_ImageView The image view to use.
     * @return A `Result` containing the created `Image` or an error.
     */
    Result<Image> CreateImage(VkImage p_Image, VkImageView p_ImageView = VK_NULL_HANDLE) const noexcept;

    const LogicalDevice::Proxy &GetDevice() const noexcept;

    void DestroyImage(const Image &p_Image) const noexcept;
    void SubmitImageForDeletion(const Image &p_Image, DeletionQueue &p_Queue) const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VmaAllocator m_Allocator = VK_NULL_HANDLE;
};
} // namespace VKit
