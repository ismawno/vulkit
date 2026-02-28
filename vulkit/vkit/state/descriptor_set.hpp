#pragma once

#ifndef VKIT_ENABLE_DESCRIPTORS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_DESCRIPTORS"
#endif

#include "vkit/state/descriptor_set_layout.hpp"
#include "tkit/container/span.hpp"

namespace VKit
{
class DeviceBuffer;

class DescriptorSet
{
  public:
    class Writer
    {
      public:
        Writer(const ProxyDevice &device, const DescriptorSetLayout *layout) : m_Device(device), m_Layout(layout)
        {
        }

        void WriteBuffer(u32 binding, const VkDescriptorBufferInfo *bufferInfo);
        void WriteImage(u32 binding, const VkDescriptorImageInfo *imageInfo);
        void Overwrite(const VkDescriptorSet set);

      private:
        ProxyDevice m_Device;
        const DescriptorSetLayout *m_Layout;

        TKit::TierArray<VkWriteDescriptorSet> m_Writes;
    };

    DescriptorSet() = default;
    DescriptorSet(const ProxyDevice &device, const VkDescriptorSet set) : m_Device(device), m_Set(set)
    {
    }

    void Bind(const VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint, VkPipelineLayout layout,
              TKit::Span<const u32> dynamicOffsets = {}) const;

    static void Bind(const ProxyDevice &device, const VkCommandBuffer commandBuffer,
                     TKit::Span<const VkDescriptorSet> sets, VkPipelineBindPoint bindPoint, VkPipelineLayout layout,
                     u32 firstSet = 0, TKit::Span<const u32> dynamicOffsets = {});

    VKIT_SET_DEBUG_NAME(m_Set, VK_OBJECT_TYPE_DESCRIPTOR_SET)

    const ProxyDevice &GetDevice() const
    {
        return m_Device;
    }
    VkDescriptorSet GetHandle() const
    {
        return m_Set;
    }
    operator VkDescriptorSet() const
    {
        return m_Set;
    }
    operator bool() const
    {
        return m_Set != VK_NULL_HANDLE;
    }

  private:
    ProxyDevice m_Device{};
    VkDescriptorSet m_Set = VK_NULL_HANDLE;
};
} // namespace VKit
