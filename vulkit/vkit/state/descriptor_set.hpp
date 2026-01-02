#pragma once

#ifndef VKIT_ENABLE_DESCRIPTORS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_DESCRIPTORS"
#endif

#include "vkit/state/descriptor_set_layout.hpp"

namespace VKit
{
class DeviceBuffer;

class VKIT_API DescriptorSet
{
  public:
    class Writer
    {
      public:
        Writer(const ProxyDevice &p_Device, const DescriptorSetLayout *p_Layout)
            : m_Device(p_Device), m_Layout(p_Layout)
        {
        }

        void WriteBuffer(u32 p_Binding, const VkDescriptorBufferInfo &p_BufferInfo);
        void WriteBuffer(u32 p_Binding, const DeviceBuffer &p_Buffer);
        void WriteImage(u32 p_Binding, const VkDescriptorImageInfo &p_ImageInfo);
        void Overwrite(const VkDescriptorSet p_Set);

      private:
        ProxyDevice m_Device;
        const DescriptorSetLayout *m_Layout;

        TKit::Array16<VkWriteDescriptorSet> m_Writes;
    };

    static Result<DescriptorSet> Create(const ProxyDevice &p_Device, VkDescriptorSet p_Set);

    DescriptorSet() = default;
    DescriptorSet(const ProxyDevice &p_Device, const VkDescriptorSet p_Set) : m_Device(p_Device), m_Set(p_Set)
    {
    }

    void Bind(const VkCommandBuffer p_CommandBuffer, VkPipelineBindPoint p_BindPoint, VkPipelineLayout p_Layout,
              TKit::Span<const u32> p_DynamicOffsets = {}) const;

    static void Bind(const ProxyDevice &p_Device, const VkCommandBuffer p_CommandBuffer,
                     TKit::Span<const VkDescriptorSet> p_Sets, VkPipelineBindPoint p_BindPoint,
                     VkPipelineLayout p_Layout, u32 p_FirstSet = 0, TKit::Span<const u32> p_DynamicOffsets = {});

    static void Bind(const ProxyDevice &p_Device, const VkCommandBuffer p_CommandBuffer, VkDescriptorSet p_Set,
                     VkPipelineBindPoint p_BindPoint, VkPipelineLayout p_Layout, u32 p_FirstSet = 0,
                     TKit::Span<const u32> p_DynamicOffsets = {});

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
