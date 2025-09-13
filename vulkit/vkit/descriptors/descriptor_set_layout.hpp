#pragma once

#ifndef VKIT_ENABLE_DESCRIPTORS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_DESCRIPTORS"
#endif

#include "vkit/core/api.hpp"
#include "vkit/vulkan/logical_device.hpp"
#include <vulkan/vulkan.h>

namespace VKit
{
/**
 * @brief Represents a Vulkan descriptor set layout.
 *
 * Manages the layout of descriptor sets, specifying bindings, types, and
 * shader stage visibility. Provides methods for creation, destruction,
 * and deferred deletion.
 */
class VKIT_API DescriptorSetLayout
{
  public:
    /**
     * @brief A utility for creating and configuring a Vulkan descriptor set layout.
     *
     * Allows adding descriptor bindings with specific types, shader stage flags,
     * and binding counts. Simplifies the process of defining descriptor layouts.
     */
    class Builder
    {
      public:
        Builder(const LogicalDevice::Proxy &p_Device);

        /**
         * @brief Creates a descriptor set layout based on the builder's configuration.
         *
         * Returns a descriptor set layout object if the creation succeeds, or an error otherwise.
         *
         * @return A `Result` containing the created `DescriptorSetLayout` or an error.
         */
        Result<DescriptorSetLayout> Build() const;

        /**
         * @brief Adds a binding to the descriptor set layout.
         *
         * Specifies the descriptor type, shader stage visibility, and number of descriptors
         * for the binding.
         *
         * @param p_Type The type of the descriptor (e.g., uniform buffer, sampler).
         * @param p_StageFlags The shader stages that can access the descriptor.
         * @param p_Count The number of descriptors for this binding (default: 1).
         * @return A reference to the builder for chaining.
         */
        Builder &AddBinding(VkDescriptorType p_Type, VkShaderStageFlags p_StageFlags, u32 p_Count = 1);

      private:
        LogicalDevice::Proxy m_Device;
        TKit::StaticArray16<VkDescriptorSetLayoutBinding> m_Bindings;
    };

    DescriptorSetLayout() = default;
    DescriptorSetLayout(const LogicalDevice::Proxy &p_Device, VkDescriptorSetLayout p_Layout,
                        const TKit::StaticArray16<VkDescriptorSetLayoutBinding> &p_Bindings);

    void Destroy();
    void SubmitForDeletion(DeletionQueue &p_Queue) const;

    const LogicalDevice::Proxy &GetDevice() const;
    VkDescriptorSetLayout GetHandle() const;
    operator VkDescriptorSetLayout() const;
    operator bool() const;

    const TKit::StaticArray16<VkDescriptorSetLayoutBinding> &GetBindings() const;

  private:
    LogicalDevice::Proxy m_Device{};
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    TKit::StaticArray16<VkDescriptorSetLayoutBinding> m_Bindings;
};
} // namespace VKit
