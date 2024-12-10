#pragma once

#include "vkit/descriptors/descriptor_set_layout.hpp"
#include "vkit/descriptors/descriptor_pool.hpp"

namespace VKit
{
/**
 * @brief Simplifies the process of writing descriptor sets.
 *
 * Designed for one-time use, this class provides methods to write buffers
 * and images into descriptor bindings and to build or overwrite descriptor sets.
 */
class VKIT_API DescriptorWriter
{
  public:
    DescriptorWriter(const LogicalDevice::Proxy &p_Device, const DescriptorSetLayout *p_Layout,
                     const DescriptorPool *p_Pool) noexcept;

    /**
     * @brief Writes a buffer to a descriptor set binding.
     *
     * Binds a buffer resource to the specified binding in the descriptor set.
     *
     * @param p_Binding The binding index in the descriptor set layout.
     * @param p_BufferInfo A pointer to the buffer descriptor info.
     */
    void WriteBuffer(u32 p_Binding, const VkDescriptorBufferInfo *p_BufferInfo) noexcept;

    /**
     * @brief Writes an image to a descriptor set binding.
     *
     * Binds an image resource to the specified binding in the descriptor set.
     *
     * @param p_Binding The binding index in the descriptor set layout.
     * @param p_ImageInfo A pointer to the image descriptor info.
     */
    void WriteImage(u32 p_Binding, const VkDescriptorImageInfo *p_ImageInfo) noexcept;

    /**
     * @brief Builds a new descriptor set.
     *
     * Allocates a descriptor set from the associated pool and writes the specified
     * bindings into it.
     *
     * @return A result containing the created descriptor set or an error.
     */
    Result<VkDescriptorSet> Build() noexcept;

    /**
     * @brief Overwrites an existing descriptor set.
     *
     * Updates the specified descriptor set with the current binding information.
     *
     * @param p_Set The descriptor set to overwrite.
     */
    void Overwrite(VkDescriptorSet p_Set) noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    const DescriptorSetLayout *m_Layout;
    const DescriptorPool *m_Pool;
    DynamicArray<VkWriteDescriptorSet> m_Writes;
};
} // namespace VKit