#pragma once

#include "vkit/descriptors/descriptor_set_layout.hpp"

namespace VKit
{
class Buffer;
class DescriptorSet
{
  public:
    class Writer
    {
      public:
        Writer(const LogicalDevice::Proxy &p_Device, const DescriptorSetLayout *p_Layout) noexcept;

        /**
         * @brief Writes a buffer to a descriptor set binding.
         *
         * Binds a buffer resource to the specified binding in the descriptor set.
         *
         * @param p_Binding The binding index in the descriptor set layout.
         * @param p_BufferInfo A pointer to the buffer descriptor info.
         */
        void WriteBuffer(u32 p_Binding, const VkDescriptorBufferInfo &p_BufferInfo) noexcept;

        /**
         * @brief Writes a buffer to a descriptor set binding.
         *
         * Binds a buffer resource to the specified binding in the descriptor set.
         *
         * @param p_Binding The binding index in the descriptor set layout.
         * @param p_Buffer A reference to the buffer to bind.
         */
        void WriteBuffer(u32 p_Binding, const Buffer &p_Buffer) noexcept;

        /**
         * @brief Writes an image to a descriptor set binding.
         *
         * Binds an image resource to the specified binding in the descriptor set.
         *
         * @param p_Binding The binding index in the descriptor set layout.
         * @param p_ImageInfo A pointer to the image descriptor info.
         */
        void WriteImage(u32 p_Binding, const VkDescriptorImageInfo &p_ImageInfo) noexcept;

        /**
         * @brief Overwrites an existing descriptor set.
         *
         * Updates the specified descriptor set with the current binding information.
         *
         * @param p_Set The descriptor set to overwrite.
         */
        void Overwrite(const VkDescriptorSet p_Set) noexcept;

      private:
        LogicalDevice::Proxy m_Device;
        const DescriptorSetLayout *m_Layout;

        TKit::StaticArray16<VkWriteDescriptorSet> m_Writes;
    };

    DescriptorSet() noexcept = default;
    DescriptorSet(VkDescriptorSet p_Set, VkDescriptorSetLayout p_Layout) noexcept;

    void Bind(const VkCommandBuffer p_CommandBuffer, VkPipelineBindPoint p_BindPoint, VkPipelineLayout p_Layout,
              std::span<const u32> p_DynamicOffsets = {}) const noexcept;

    static void Bind(const VkCommandBuffer p_CommandBuffer, std::span<const VkDescriptorSet> p_Sets,
                     VkPipelineBindPoint p_BindPoint, VkPipelineLayout p_Layout, u32 p_FirstSet = 0,
                     std::span<const u32> p_DynamicOffsets = {}) noexcept;

    static void Bind(const VkCommandBuffer p_CommandBuffer, VkDescriptorSet p_Set, VkPipelineBindPoint p_BindPoint,
                     VkPipelineLayout p_Layout, u32 p_FirstSet = 0,
                     std::span<const u32> p_DynamicOffsets = {}) noexcept;

    VkDescriptorSet GetDescriptorSet() const noexcept;
    VkDescriptorSetLayout GetLayout() const noexcept;

    explicit(false) operator VkDescriptorSet() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    VkDescriptorSet m_Set = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
};
} // namespace VKit