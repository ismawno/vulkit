#pragma once

#include "vkit/core/alias.hpp"
#include "vkit/core/device.hpp"
#include "vkit/core/vma.hpp"

#include "tkit/core/non_copyable.hpp"
#include "tkit/memory/block_allocator.hpp"

#include <vulkan/vulkan.hpp>

namespace VKit
{
class VKIT_API Buffer
{
    TKIT_NON_COPYABLE(Buffer)
  public:
    struct Specs
    {
        VkDeviceSize InstanceCount;
        VkDeviceSize InstanceSize;
        VkBufferUsageFlags Usage;
        VmaAllocationCreateInfo AllocationInfo;
        VkDeviceSize MinimumAlignment = 1;
    };

    Buffer(const Specs &p_Specs) noexcept;

    ~Buffer() noexcept;

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

    void CopyFrom(const Buffer &p_Source) noexcept;

    VkBuffer GetBuffer() const noexcept;

    VkDeviceSize GetSize() const noexcept;
    VkDeviceSize GetInstanceSize() const noexcept;
    VkDeviceSize GetInstanceCount() const noexcept;

  private:
    void createBuffer(VkBufferUsageFlags p_Usage, const VmaAllocationCreateInfo &p_AllocationInfo) noexcept;

    TKit::Ref<Device> m_Device;
    void *m_Data = nullptr;

    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VmaAllocation m_Allocation = VK_NULL_HANDLE;

    VkDeviceSize m_InstanceSize;
    VkDeviceSize m_AlignedInstanceSize;
    VkDeviceSize m_Size;
};
} // namespace VKit