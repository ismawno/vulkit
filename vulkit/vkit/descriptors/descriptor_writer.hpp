#pragma once

#include "vkit/descriptors/descriptor_set_layout.hpp"
#include "vkit/descriptors/descriptor_pool.hpp"

namespace VKit
{
// Meant to be used on the spot, not stored
class VKIT_API DescriptorWriter
{
  public:
    DescriptorWriter(const LogicalDevice::Proxy &p_Device, const DescriptorSetLayout *p_Layout,
                     const DescriptorPool *p_Pool) noexcept;

    void WriteBuffer(u32 p_Binding, const VkDescriptorBufferInfo *p_BufferInfo) noexcept;
    void WriteImage(u32 p_Binding, const VkDescriptorImageInfo *p_ImageInfo) noexcept;

    Result<VkDescriptorSet> Build() noexcept;
    void Overwrite(VkDescriptorSet p_Set) noexcept;

  private:
    LogicalDevice::Proxy m_Device;
    const DescriptorSetLayout *m_Layout;
    const DescriptorPool *m_Pool;
    DynamicArray<VkWriteDescriptorSet> m_Writes;
};
} // namespace VKit