#pragma once

#ifndef VKIT_ENABLE_IMAGE
#    error "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_IMAGE"
#endif

#include "vkit/device/logical_device.hpp"
#include "vkit/memory/allocator.hpp"
#include "vkit/resource/utils.hpp"

namespace VKit
{
class CommandPool;
class Buffer;

using ImageFlags = u8;
enum ImageFlagBits : ImageFlags
{
    ImageFlag_ColorAttachment = 1 << 0,
    ImageFlag_DepthAttachment = 1 << 1,
    ImageFlag_StencilAttachment = 1 << 2,
    ImageFlag_InputAttachment = 1 << 3,
    ImageFlag_Sampled = 1 << 4,
    ImageFlag_ForceHostVisible = 1 << 5
};
class Image
{
  public:
    class Builder
    {
      public:
        Builder(const ProxyDevice &p_Device, VmaAllocator p_Allocator, const VkExtent3D &p_Extent,
                VkFormat p_Format, ImageFlags p_Flags = 0);
        Builder(const ProxyDevice &p_Device, VmaAllocator p_Allocator, const VkExtent2D &p_Extent,
                VkFormat p_Format, ImageFlags p_Flags = 0);

        /**
         * @brief Creates a vulkan image with the provided specification.
         *
         * IMPORTANT: Image view configuration should be done after the image configuration is finished. That is, try to
         * call `CreateImageView()` as the last method before `Build()`.
         *
         * @return A `Result` containing the created `Image` or an error if the creation fails.
         */
        Result<Image> Build() const;

        Builder &SetImageType(VkImageType p_Type);
        Builder &SetDepth(u32 p_Depth);
        Builder &SetMipLevels(u32 p_Levels);
        Builder &SetArrayLayers(u32 p_Layers);
        Builder &SetTiling(VkImageTiling p_Tiling);
        Builder &SetInitialLayout(VkImageLayout p_Layout);
        Builder &SetSamples(VkSampleCountFlagBits p_Samples);
        Builder &SetSharingMode(VkSharingMode p_Mode);
        Builder &SetFlags(VkImageCreateFlags p_Flags);
        Builder &SetUsage(VkImageUsageFlags p_Flags);
        Builder &SetImageCreateInfo(const VkImageCreateInfo &p_Info);

        Builder &WithImageView();
        Builder &WithImageView(const VkImageViewCreateInfo &p_Info);
        Builder &WithImageView(const VkImageSubresourceRange &p_Range);

      private:
        ProxyDevice m_Device;
        VmaAllocator m_Allocator;
        VkImageCreateInfo m_ImageInfo{};
        VkImageViewCreateInfo m_ViewInfo{};
        ImageFlags m_Flags;
    };

    struct HostData
    {
        u8 *Data;
        u32 Width;
        u32 Height;
        u32 Depth;
        u32 Channels;
    };

    struct Info
    {
        VmaAllocator Allocator;
        VmaAllocation Allocation;
        VkFormat Format;
        u32 Width;
        u32 Height;
        u32 Depth;
        ImageFlags Flags;
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

    static Info FromSwapChain(VkFormat p_Format, const VkExtent2D &p_Extent,
                              ImageFlags p_Flags = ImageFlag_ColorAttachment);

    Image() = default;
    Image(const ProxyDevice &p_Device, const VkImage p_Image, const VkImageLayout p_Layout, const Info &p_Info,
          const VkImageView p_ImageView = VK_NULL_HANDLE)
        : m_Device(p_Device), m_Image(p_Image), m_ImageView(p_ImageView), m_Layout(p_Layout), m_Info(p_Info)
    {
    }

    Result<VkImageView> CreateImageView();
    Result<VkImageView> CreateImageView(const VkImageViewCreateInfo &p_Info);
    Result<VkImageView> CreateImageView(const VkImageSubresourceRange &p_Range);

    void TransitionLayout(VkCommandBuffer p_CommandBuffer, VkImageLayout p_Layout, const TransitionInfo &p_Info);

    void CopyFromImage(VkCommandBuffer p_CommandBuffer, const Image &p_Source, const ImageCopy &p_Info = {});

    Result<> CopyFromImage(CommandPool &p_Pool, VkQueue p_Queue, const Image &p_Source, const ImageCopy &p_Info = {});

    void CopyFromBuffer(VkCommandBuffer p_CommandBuffer, const Buffer &p_Source, const BufferImageCopy &p_Info = {});

    Result<> CopyFromBuffer(CommandPool &p_Pool, VkQueue p_Queue, const Buffer &p_Source,
                            const BufferImageCopy &p_Info = {});

    Result<> UploadFromHost(CommandPool &p_Pool, VkQueue p_Queue, const HostData &p_Data,
                            VkImageLayout p_FinalLayout = VK_IMAGE_LAYOUT_UNDEFINED);

    VkDeviceSize ComputeSize(u32 p_Width, u32 p_Height, u32 p_Mip = 0, u32 p_Depth = 1) const;
    VkDeviceSize ComputeSize(u32 p_Mip = 0) const;
    static VkDeviceSize ComputeSize(VkFormat p_Format, u32 p_Width, u32 p_Height, u32 p_Mip = 0, u32 p_Depth = 1);

    VkDeviceSize GetBytesPerPixel() const
    {
        return GetBytesPerPixel(m_Info.Format);
    }
    static VkDeviceSize GetBytesPerPixel(VkFormat p_Format);

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

namespace VKit::Detail
{
VkImageAspectFlags DeduceAspectMask(const ImageFlags p_Flags);
}
