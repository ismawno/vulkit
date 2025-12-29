#pragma once

#ifndef VKIT_ENABLE_DESCRIPTORS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_DESCRIPTORS"
#endif

#include "vkit/descriptors/descriptor_set_layout.hpp"

namespace VKit
{
class Buffer;

/**
 * @brief Represents a Vulkan descriptor set.
 *
 * Manages the binding of resources to a descriptor set, providing methods for
 * binding buffers and images to the set. It is an extremely thin wrapper.
 *
 */
class VKIT_API DescriptorSet
{
  public:
    class Writer
    {
      public:
        Writer(const LogicalDevice::Proxy &p_Device, const DescriptorSetLayout *p_Layout)
            : m_Device(p_Device), m_Layout(p_Layout)
        {
        }

        /**
         * @brief Writes a buffer to a descriptor set binding.
         *
         * Binds a buffer resource to the specified binding in the descriptor set.
         *
         * @param p_Binding The binding index in the descriptor set layout.
         * @param p_BufferInfo A pointer to the buffer descriptor info.
         */
        void WriteBuffer(u32 p_Binding, const VkDescriptorBufferInfo &p_BufferInfo);

        /**
         * @brief Writes a buffer to a descriptor set binding.
         *
         * Binds a buffer resource to the specified binding in the descriptor set.
         *
         * @param p_Binding The binding index in the descriptor set layout.
         * @param p_Buffer A reference to the buffer to bind.
         */
        void WriteBuffer(u32 p_Binding, const Buffer &p_Buffer);

        /**
         * @brief Writes an image to a descriptor set binding.
         *
         * Binds an image resource to the specified binding in the descriptor set.
         *
         * @param p_Binding The binding index in the descriptor set layout.
         * @param p_ImageInfo A pointer to the image descriptor info.
         */
        void WriteImage(u32 p_Binding, const VkDescriptorImageInfo &p_ImageInfo);

        /**
         * @brief Overwrites an existing descriptor set.
         *
         * Updates the specified descriptor set with the current binding information.
         *
         * @param p_Set The descriptor set to overwrite.
         */
        void Overwrite(const VkDescriptorSet p_Set);

      private:
        LogicalDevice::Proxy m_Device;
        const DescriptorSetLayout *m_Layout;

        TKit::Array16<VkWriteDescriptorSet> m_Writes;
    };

    static Result<DescriptorSet> Create(const LogicalDevice::Proxy &p_Device, VkDescriptorSet p_Set);

    DescriptorSet() = default;
    DescriptorSet(const LogicalDevice::Proxy &p_Device, const VkDescriptorSet p_Set) : m_Device(p_Device), m_Set(p_Set)
    {
    }

    void Bind(const VkCommandBuffer p_CommandBuffer, VkPipelineBindPoint p_BindPoint, VkPipelineLayout p_Layout,
              TKit::Span<const u32> p_DynamicOffsets = {}) const;

    static void Bind(const LogicalDevice::Proxy &p_Device, const VkCommandBuffer p_CommandBuffer,
                     TKit::Span<const VkDescriptorSet> p_Sets, VkPipelineBindPoint p_BindPoint,
                     VkPipelineLayout p_Layout, u32 p_FirstSet = 0, TKit::Span<const u32> p_DynamicOffsets = {});

    static void Bind(const LogicalDevice::Proxy &p_Device, const VkCommandBuffer p_CommandBuffer, VkDescriptorSet p_Set,
                     VkPipelineBindPoint p_BindPoint, VkPipelineLayout p_Layout, u32 p_FirstSet = 0,
                     TKit::Span<const u32> p_DynamicOffsets = {});

    const LogicalDevice::Proxy &GetDevice() const
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
    LogicalDevice::Proxy m_Device{};
    VkDescriptorSet m_Set = VK_NULL_HANDLE;
};
} // namespace VKit
