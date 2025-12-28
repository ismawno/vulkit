#pragma once

#include "vkit/core/alias.hpp"
#include "tkit/utils/limits.hpp"
#include <vulkan/vulkan.h>
namespace VKit
{
// reinventing the wheel here for the sake of defaults
struct BufferCopy
{
    VkDeviceSize Size = VK_WHOLE_SIZE;
    VkDeviceSize SrcOffset = 0;
    VkDeviceSize DstOffset = 0;
};

#define FULL TKIT_U32_MAX

struct ImageCopy
{
    VkImageSubresourceLayers SrcSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_NONE, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1};
    VkOffset3D SrcOffset = {0, 0, 0};
    VkImageSubresourceLayers DstSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_NONE, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1};
    VkOffset3D DstOffset = {0, 0, 0};

    // defaults to full image
    VkExtent3D Extent{FULL, FULL, FULL};
};

struct BufferImageCopy
{
    VkDeviceSize BufferOffset = 0;
    u32 BufferRowLength = 0;
    u32 BufferImageHeight = 0;

    // by setting none one is chosen automatically
    VkImageSubresourceLayers Subresource = {
        .aspectMask = VK_IMAGE_ASPECT_NONE, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1};
    VkOffset3D ImageOffset = {0, 0, 0};

    // defaults to full image
    VkExtent3D Extent{FULL, FULL, FULL};
};

} // namespace VKit

#undef FULL
