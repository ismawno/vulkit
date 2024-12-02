#pragma once

#include "vkit/backend/system.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/core/vma.hpp"

#include "tkit/memory/block_allocator.hpp"

#include <vulkan/vulkan.hpp>

namespace VKit
{
class CommandPool;
class VKIT_API Buffer
{
  public:
    struct Specs
    {
        VmaAllocator Allocator = VK_NULL_HANDLE;
        VkDeviceSize InstanceCount;
        VkDeviceSize InstanceSize;
        VkBufferUsageFlags Usage;
        VmaAllocationCreateInfo AllocationInfo;
        VkDeviceSize MinimumAlignment = 1;
    };

    struct Info
    {
        VmaAllocator Allocator;
        VmaAllocation Allocation;

        VkDeviceSize InstanceSize;
        VkDeviceSize InstanceCount;
        VkDeviceSize AlignedInstanceSize;
        VkDeviceSize Size;
    };

    static RawResult<Buffer> Create(const Specs &p_Specs) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) noexcept;

    void Map() noexcept;
    void Unmap() noexcept;

    void Write(const void *p_Data, VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0) noexcept;
    void WriteAt(usize p_Index, const void *p_Data) noexcept;

    void Flush(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0) noexcept;
    void FlushAt(usize p_Index) noexcept;

    void Invalidate(VkDeviceSize p_Size = VK_WHOLE_SIZE, VkDeviceSize p_Offset = 0) noexcept;
    void InvalidateAt(usize p_Index) noexcept;

    VkDescriptorBufferInfo GetDescriptorInfo(VkDeviceSize p_Size = VK_WHOLE_SIZE,
                                             VkDeviceSize p_Offset = 0) const noexcept;
    VkDescriptorBufferInfo GetDescriptorInfoAt(usize p_Index) const noexcept;

    void *GetData() const noexcept;
    void *ReadAt(usize p_Index) const noexcept;

    VulkanRawResult CopyFrom(const Buffer &p_Source, CommandPool &p_Pool, VkQueue p_Queue) noexcept;

    VkBuffer GetBuffer() const noexcept;

    const Info &GetInfo() const noexcept;

  private:
    explicit Buffer(VkBuffer p_Buffer, const Info &p_Info) noexcept;

    void *m_Data = nullptr;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit