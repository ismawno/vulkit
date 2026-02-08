#pragma once

#ifndef VKIT_ENABLE_DEVICE_IMAGE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_DEVICE_IMAGE"
#endif

#include "vkit/memory/allocator.hpp"

namespace VKit
{
class CommandPool;
class DeviceBuffer;

using DeviceImageFlags = u16;
enum DeviceImageFlagBits : DeviceImageFlags
{
    DeviceImageFlag_Color = 1 << 0,
    DeviceImageFlag_Depth = 1 << 1,
    DeviceImageFlag_Stencil = 1 << 2,
    DeviceImageFlag_ColorAttachment = 1 << 3,
    DeviceImageFlag_DepthAttachment = 1 << 4,
    DeviceImageFlag_StencilAttachment = 1 << 5,
    DeviceImageFlag_InputAttachment = 1 << 6,
    DeviceImageFlag_Sampled = 1 << 7,
    DeviceImageFlag_ForceHostVisible = 1 << 8,
    DeviceImageFlag_Source = 1 << 9,
    DeviceImageFlag_Destination = 1 << 10,
};

} // namespace VKit

namespace VKit
{

class DeviceImage
{
  public:
    class Builder
    {
      public:
        Builder(const ProxyDevice &device, VmaAllocator allocator, const VkExtent3D &extent, VkFormat format,
                DeviceImageFlags flags = 0);
        Builder(const ProxyDevice &device, VmaAllocator allocator, const VkExtent2D &extent, VkFormat format,
                DeviceImageFlags flags = 0);

        /**
         * @brief Creates a vulkan image with the provided specification.
         *
         * IMPORTANT: Image view configuration should be done after the image configuration is finished. That is, try to
         * call `CreateImageView()` as the last method before `Build()`.
         *
         * @return A `Result` containing the created `DeviceImage` or an error if the creation fails.
         */
        VKIT_NO_DISCARD Result<DeviceImage> Build() const;

        Builder &SetImageType(VkImageType type);
        Builder &SetDepth(u32 depth);
        Builder &SetMipLevels(u32 levels);
        Builder &SetArrayLayers(u32 layers);
        Builder &SetTiling(VkImageTiling tiling);
        Builder &SetInitialLayout(VkImageLayout layout);
        Builder &SetSamples(VkSampleCountFlagBits samples);
        Builder &SetSharingMode(VkSharingMode mode);
        Builder &SetFlags(VkImageCreateFlags flags);
        Builder &SetUsage(VkImageUsageFlags flags);
        Builder &SetImageCreateInfo(const VkImageCreateInfo &info);
        Builder &SetNextToImageInfo(const void *next);
        Builder &SetNextToImageViewInfo(const void *next);

        const VkImageCreateInfo &GetImageInfo() const;
        const VkImageViewCreateInfo &GetImageViewInfo() const;

        Builder &WithImageView();
        Builder &WithImageView(const VkImageViewCreateInfo &info);
        Builder &WithImageView(const VkImageSubresourceRange &range);

      private:
        ProxyDevice m_Device;
        VmaAllocator m_Allocator;
        VkImageCreateInfo m_ImageInfo{};
        VkImageViewCreateInfo m_ViewInfo{};
        DeviceImageFlags m_Flags;
    };

    struct Info
    {
        VmaAllocator Allocator;
        VmaAllocation Allocation;
        VkFormat Format;
        u32 Width;
        u32 Height;
        u32 Depth;
        DeviceImageFlags Flags;
    };

    struct TransitionInfo
    {
        u32 SrcFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        u32 DstFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        VkAccessFlags SrcAccess = 0;
        VkAccessFlags DstAccess = 0;
        VkPipelineStageFlags SrcStage = 0;
        VkPipelineStageFlags DstStage = 0;
        // VK_IMAGE_ASPECT_NONE means one will be chosen automatically
        VkImageSubresourceRange Range{VK_IMAGE_ASPECT_NONE, 0, 1, 0, 1};
    };

#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_synchronization2)
    struct TransitionInfo2
    {
        u32 SrcFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        u32 DstFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        VkAccessFlags2KHR SrcAccess = 0;
        VkAccessFlags2KHR DstAccess = 0;
        VkPipelineStageFlags2KHR SrcStage = 0;
        VkPipelineStageFlags2KHR DstStage = 0;
        // VK_IMAGE_ASPECT_NONE means one will be chosen automatically
        VkImageSubresourceRange Range{VK_IMAGE_ASPECT_NONE, 0, 1, 0, 1};
    };
#endif

    static Info FromSwapChain(VkFormat format, const VkExtent2D &extent,
                              DeviceImageFlags flags = DeviceImageFlag_ColorAttachment);

    DeviceImage() = default;
    DeviceImage(const ProxyDevice &device, const VkImage image, const VkImageLayout layout, const Info &info,
                const VkImageView imageView = VK_NULL_HANDLE)
        : m_Device(device), m_Image(image), m_ImageView(imageView), m_Layout(layout), m_Info(info)
    {
    }

    VKIT_NO_DISCARD Result<VkImageView> CreateImageView();
    VKIT_NO_DISCARD Result<VkImageView> CreateImageView(const VkImageViewCreateInfo &info);
    VKIT_NO_DISCARD Result<VkImageView> CreateImageView(const VkImageSubresourceRange &range);

    VkImageMemoryBarrier CreateTransitionLayoutBarrier(VkImageLayout layout, const TransitionInfo &info,
                                                       const void *next = nullptr) const;
    void TransitionLayout(VkCommandBuffer commandBuffer, VkImageLayout layout, const TransitionInfo &info,
                          const void *barrierNext = nullptr);

    void CopyFromImage(VkCommandBuffer commandBuffer, const DeviceImage &source, TKit::Span<const VkImageCopy> copy);
    void CopyFromBuffer(VkCommandBuffer commandBuffer, const DeviceBuffer &source,
                        TKit::Span<const VkBufferImageCopy> copy);

#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_synchronization2)
    VkImageMemoryBarrier2KHR CreateTransitionLayoutBarrier2(VkImageLayout layout, const TransitionInfo2 &info,
                                                            const void *barrierNext = nullptr) const;

    void TransitionLayout2(VkCommandBuffer commandBuffer, VkImageLayout layout, const TransitionInfo2 &info,
                           VkDependencyFlags flags = 0, const void *depNext = nullptr);

    void CopyFromImage2(VkCommandBuffer commandBuffer, const DeviceImage &source,
                        TKit::Span<const VkImageCopy2KHR> copy, const void *next = nullptr);
    void CopyFromBuffer2(VkCommandBuffer commandBuffer, const DeviceBuffer &source,
                         TKit::Span<const VkBufferImageCopy2KHR> copy, const void *next = nullptr);
#endif

    VKIT_NO_DISCARD Result<> CopyFromImage(CommandPool &pool, VkQueue queue, const DeviceImage &source,
                                           TKit::Span<const VkImageCopy> copy);
    VKIT_NO_DISCARD Result<> CopyFromBuffer(CommandPool &pool, VkQueue queue, const DeviceBuffer &source,
                                            TKit::Span<const VkBufferImageCopy> copy);

    VkDeviceSize ComputeSize(u32 width, u32 height, u32 mip = 0, u32 depth = 1) const;
    VkDeviceSize ComputeSize(u32 mip = 0) const;
    static VkDeviceSize ComputeSize(VkFormat format, u32 width, u32 height, u32 mip = 0, u32 depth = 1);

    VkDeviceSize GetBytesPerPixel() const
    {
        return GetBytesPerPixel(m_Info.Format);
    }
    static VkDeviceSize GetBytesPerPixel(VkFormat format);

    void Destroy();
    void DestroyImageView();

    operator VkImage() const
    {
        return m_Image;
    }
    operator bool() const
    {
        return m_Image != VK_NULL_HANDLE;
    }

    VkImageAspectFlags InferAspectMask() const;

    const ProxyDevice &GetDevice() const
    {
        return m_Device;
    }
    VkImage GetHandle() const
    {
        return m_Image;
    }
    VkImageView GetImageView() const
    {
        return m_ImageView;
    }
    VkImageLayout GetLayout() const
    {
        return m_Layout;
    }
    const Info &GetInfo() const
    {
        return m_Info;
    }

  private:
    ProxyDevice m_Device{};
    VkImage m_Image = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkImageLayout m_Layout;
    Info m_Info;
};
} // namespace VKit
