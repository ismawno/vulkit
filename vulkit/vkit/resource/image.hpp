#pragma once

#ifndef VKIT_ENABLE_IMAGE
#    error "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_IMAGE"
#endif

#include "vkit/vulkan/logical_device.hpp"
#include "vkit/vulkan/allocator.hpp"
#include "vkit/resource/utils.hpp"

namespace VKit
{
class CommandPool;
class Buffer;
class Image
{
  public:
    using Flags = u8;
    enum FlagBits : Flags
    {
        Flag_None = 0,
        Flag_ColorAttachment = 1 << 0,
        Flag_DepthAttachment = 1 << 1,
        Flag_StencilAttachment = 1 << 2,
        Flag_InputAttachment = 1 << 3,
        Flag_Sampled = 1 << 4,
        Flag_ForceHostVisible = 1 << 5
    };
    class Builder
    {
      public:
        Builder(const LogicalDevice::Proxy &p_Device, VmaAllocator p_Allocator, const VkExtent3D &p_Extent,
                VkFormat p_Format, Flags p_Flags = 0);
        Builder(const LogicalDevice::Proxy &p_Device, VmaAllocator p_Allocator, const VkExtent2D &p_Extent,
                VkFormat p_Format, Flags p_Flags = 0);

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
        LogicalDevice::Proxy m_Device;
        VmaAllocator m_Allocator;
        VkImageCreateInfo m_ImageInfo;
        VkImageViewCreateInfo m_ViewInfo;
        Flags m_Flags;
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
        Flags Flags;
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

    static Info FromSwapChain(VkFormat p_Format, const VkExtent2D &p_Extent, Flags p_Flags = Flag_ColorAttachment);

    Image() = default;
    Image(const LogicalDevice::Proxy &p_Device, const VkImage p_Image, const VkImageLayout p_Layout, const Info &p_Info,
          const VkImageView p_ImageView = VK_NULL_HANDLE)
        : m_Device(p_Device), m_Image(p_Image), m_ImageView(p_ImageView), m_Layout(p_Layout), m_Info(p_Info)
    {
    }

    Result<VkImageView> CreateImageView();
    Result<VkImageView> CreateImageView(const VkImageViewCreateInfo &p_Info);
    Result<VkImageView> CreateImageView(const VkImageSubresourceRange &p_Range);

    void TransitionLayout(VkCommandBuffer p_CommandBuffer, VkImageLayout p_Layout, const TransitionInfo &p_Info);

    /**
     * @brief Copies data from another image into this image.
     *
     * Records the copy into a command buffer.
     *
     * @param p_CommandBuffer The command buffer to which the copy will be recorded.
     * @param p_Source The source image to copy from.
     * @param p_Info Information about the range of the copy.
     */
    void CopyFromImage(VkCommandBuffer p_CommandBuffer, const Image &p_Source, const ImageCopy &p_Info = {});

    /**
     * @brief Copies data from another image into this image.
     *
     * Records the copy into a command buffer.
     *
     * @param p_CommandBuffer The command buffer to which the copy will be recorded.
     * @param p_Source The source image to copy from.
     * @param p_Info Information about the range of the copy.
     * @return A `Result` indicating success or failure.
     */
    Result<> CopyFromImage(CommandPool &p_Pool, VkQueue p_Queue, const Image &p_Source, const ImageCopy &p_Info = {});

    /**
     * @brief Copies data from a buffer into this image.
     *
     * Records the copy into a command buffer.
     *
     * @param p_CommandBuffer The command buffer to which the copy will be recorded.
     * @param p_Source The source buffer to copy from.
     * @param p_Info Information about the range of the copy.
     */
    void CopyFromBuffer(VkCommandBuffer p_CommandBuffer, const Buffer &p_Source, const BufferImageCopy &p_Info = {});

    /**
     * @brief Copies data from a buffer into this image.
     *
     * Uses a command pool and queue to perform the buffer-to-buffer copy operation.
     *
     * @param p_Pool The command pool to allocate the copy command.
     * @param p_Queue The queue to submit the copy command.
     * @param p_Source The source buffer to copy from.
     * @param p_Info Information about the range of the copy.
     * @return A `Result` indicating success or failure.
     */
    Result<> CopyFromBuffer(CommandPool &p_Pool, VkQueue p_Queue, const Buffer &p_Source,
                            const BufferImageCopy &p_Info = {});

    /**
     * @brief Uploads host data to the buffer, offsetted and up to the specified size, which must not exceed the
     * buffer's.
     *
     * This method is designed to be used for device local buffers.
     *
     * @param p_Pool The command pool to allocate the copy command.
     * @param p_Queue The queue to submit the copy command.
     * @param p_Data The host-side image.
     * @return A `Result` indicating success or failure.
     */
    Result<> UploadFromHost(CommandPool &p_Pool, VkQueue p_Queue, const HostData &p_Data,
                            VkImageLayout p_FinalLayout = VK_IMAGE_LAYOUT_UNDEFINED);

    VkDeviceSize GetSize() const
    {
        return GetSize(m_Info.Width, m_Info.Height);
    }
    VkDeviceSize GetSize(u32 p_BufferRowLength, u32 p_BufferImageHeight) const;

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

    const LogicalDevice::Proxy &GetDevice() const
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
    LogicalDevice::Proxy m_Device{};
    VkImage m_Image = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkImageLayout m_Layout;
    Info m_Info;
};
namespace Detail
{
VkImageAspectFlags DeduceAspectMask(const Image::Flags p_Flags);
}
} // namespace VKit
