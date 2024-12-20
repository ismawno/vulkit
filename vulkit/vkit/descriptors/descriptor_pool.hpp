#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/backend/logical_device.hpp"
#include "vkit/descriptors/descriptor_set.hpp"
#include "tkit/profiling/vulkan.hpp"

#include <vulkan/vulkan.hpp>
#include <span>

namespace VKit
{
/**
 * @brief Manages a Vulkan descriptor pool and its allocations.
 *
 * Handles the creation, allocation, and deallocation of descriptor sets.
 * Also supports resetting the pool for reallocation of resources.
 */
class VKIT_API DescriptorPool
{
  public:
    /**
     * @brief A utility for creating and configuring a Vulkan descriptor pool.
     *
     * Provides methods to specify the maximum number of sets, pool sizes, and creation flags.
     * Supports fine-grained control over the pool's configuration.
     */
    class Builder
    {
      public:
        explicit Builder(const LogicalDevice::Proxy &p_Device) noexcept;

        /**
         * @brief Creates a descriptor pool based on the builder's configuration.
         *
         * Returns a descriptor pool object if the creation succeeds, or an error otherwise.
         *
         * @return A result containing the created DescriptorPool or an error.
         */
        Result<DescriptorPool> Build() const noexcept;

        Builder &SetMaxSets(u32 p_MaxSets) noexcept;
        Builder &SetFlags(VkDescriptorPoolCreateFlags p_Flags) noexcept;
        Builder &AddFlags(VkDescriptorPoolCreateFlags p_Flags) noexcept;
        Builder &RemoveFlags(VkDescriptorPoolCreateFlags p_Flags) noexcept;
        Builder &AddPoolSize(VkDescriptorType p_Type, u32 p_Size) noexcept;

      private:
        LogicalDevice::Proxy m_Device;

        u32 m_MaxSets = 8;
        VkDescriptorPoolCreateFlags m_Flags = 0;
        TKit::StaticArray32<VkDescriptorPoolSize> m_PoolSizes{};
    };

    struct Info
    {
        u32 MaxSets;
        TKit::StaticArray32<VkDescriptorPoolSize> PoolSizes;
    };

    DescriptorPool() noexcept = default;
    DescriptorPool(const LogicalDevice::Proxy &p_Device, VkDescriptorPool p_Pool, const Info &p_Info) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    const Info &GetInfo() const noexcept;

    /**
     * @brief Allocates a descriptor set from the pool.
     *
     * Creates a descriptor set using the specified layout.
     *
     * @param p_Layout The descriptor set layout to use for allocation.
     * @return A result containing the allocated descriptor set or an error.
     */
    Result<DescriptorSet> Allocate(VkDescriptorSetLayout p_Layout) const noexcept;

    /**
     * @brief Deallocates one or more descriptor sets from the pool.
     *
     * Frees the specified descriptor sets, making their resources available for reallocation.
     *
     * @param p_Sets A span containing the descriptor sets to deallocate.
     */
    void Deallocate(std::span<const VkDescriptorSet> p_Sets) const noexcept;

    /**
     * @brief Deallocates a descriptor set from the pool.
     *
     * Frees the specified descriptor set, making its resources available for reallocation.
     *
     * @param p_Set The descriptor set to deallocate.
     */
    void Deallocate(VkDescriptorSet p_Set) const noexcept;
    void Reset() noexcept;

    VkDescriptorPool GetPool() const noexcept;
    explicit(false) operator VkDescriptorPool() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkDescriptorPool m_Pool = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit