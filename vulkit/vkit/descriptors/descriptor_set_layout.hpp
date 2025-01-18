#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/backend/logical_device.hpp"
#include "tkit/core/non_copyable.hpp"

#include <vulkan/vulkan.h>
#include <span>

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
        explicit Builder(const LogicalDevice::Proxy &p_Device) noexcept;

        /**
         * @brief Creates a descriptor set layout based on the builder's configuration.
         *
         * Returns a descriptor set layout object if the creation succeeds, or an error otherwise.
         *
         * @return A `Result` containing the created `DescriptorSetLayout` or an error.
         */
        Result<DescriptorSetLayout> Build() const noexcept;

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
        Builder &AddBinding(VkDescriptorType p_Type, VkShaderStageFlags p_StageFlags, u32 p_Count = 1) noexcept;

      private:
        LogicalDevice::Proxy m_Device;

        TKit::StaticArray16<VkDescriptorSetLayoutBinding> m_Bindings;
    };

    DescriptorSetLayout() noexcept = default;
    DescriptorSetLayout(const LogicalDevice::Proxy &p_Device, VkDescriptorSetLayout p_Layout,
                        const TKit::StaticArray16<VkDescriptorSetLayoutBinding> &p_Bindings) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    VkDescriptorSetLayout GetLayout() const noexcept;
    explicit(false) operator VkDescriptorSetLayout() const noexcept;
    explicit(false) operator bool() const noexcept;

    const TKit::StaticArray16<VkDescriptorSetLayoutBinding> &GetBindings() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    TKit::StaticArray16<VkDescriptorSetLayoutBinding> m_Bindings;
};
} // namespace VKit