#pragma once

#include "vkit/backend/system.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/core/vma.hpp"

#include "tkit/memory/block_allocator.hpp"

#include <vulkan/vulkan.hpp>

// User may not use mutable buffer methods if the buffer is not/cannot be mapped

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

    static Result<Buffer> Create(const Specs &p_Specs) noexcept;

    explicit Buffer(VkBuffer p_Buffer, const Info &p_Info) noexcept;

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

    VulkanResult CopyFrom(const Buffer &p_Source, CommandPool &p_Pool, VkQueue p_Queue) noexcept;

    VkBuffer GetBuffer() const noexcept;
    explicit(false) operator VkBuffer() const noexcept;
    explicit(false) operator bool() const noexcept;

    const Info &GetInfo() const noexcept;

  private:
    void *m_Data = nullptr;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit