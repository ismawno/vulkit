#pragma once

#ifndef VKIT_ENABLE_DESCRIPTORS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_DESCRIPTORS"
#endif

#include "vkit/core/api.hpp"
#include "vkit/vulkan/logical_device.hpp"
#include "vkit/descriptors/descriptor_set.hpp"

#include <vulkan/vulkan.h>

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
        Builder(const LogicalDevice::Proxy &p_Device) : m_Device(p_Device)
        {
        }

        /**
         * @brief Creates a descriptor pool based on the builder's configuration.
         *
         * Returns a descriptor pool object if the creation succeeds, or an error otherwise.
         *
         * @return A `Result` containing the created `DescriptorPool` or an error.
         */
        Result<DescriptorPool> Build() const;

        Builder &SetMaxSets(u32 p_MaxSets);
        Builder &SetFlags(VkDescriptorPoolCreateFlags p_Flags);
        Builder &AddFlags(VkDescriptorPoolCreateFlags p_Flags);
        Builder &RemoveFlags(VkDescriptorPoolCreateFlags p_Flags);
        Builder &AddPoolSize(VkDescriptorType p_Type, u32 p_Size);

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

    DescriptorPool() = default;
    DescriptorPool(const LogicalDevice::Proxy &p_Device, const VkDescriptorPool p_Pool, const Info &p_Info)
        : m_Device(p_Device), m_Pool(p_Pool), m_Info(p_Info)
    {
    }

    void Destroy();
    void SubmitForDeletion(DeletionQueue &p_Queue) const;

    const Info &GetInfo() const
    {
        return m_Info;
    }

    /**
     * @brief Allocates a descriptor set from the pool.
     *
     * Creates a descriptor set using the specified layout.
     *
     * @param p_Layout The descriptor set layout to use for allocation.
     * @return A `Result` containing the allocated descriptor set or an error.
     */
    Result<DescriptorSet> Allocate(VkDescriptorSetLayout p_Layout) const;

    /**
     * @brief Deallocates one or more descriptor sets from the pool.
     *
     * Frees the specified descriptor sets, making their resources available for reallocation.
     *
     * @param p_Sets A span containing the descriptor sets to deallocate.
     * @return A `Result` indicating success or failure.
     */
    Result<> Deallocate(TKit::Span<const VkDescriptorSet> p_Sets) const;

    /**
     * @brief Deallocates a descriptor set from the pool.
     *
     * Frees the specified descriptor set, making its resources available for reallocation.
     *
     * @param p_Set The descriptor set to deallocate.
     * @return A `Result` indicating success or failure.
     */
    Result<> Deallocate(VkDescriptorSet p_Set) const;

    /**
     * @brief Resets the descriptor pool, making all resources available for reallocation.
     *
     * @return A `Result` indicating success or failure.
     */
    Result<> Reset();

    const LogicalDevice::Proxy &GetDevice() const
    {
        return m_Device;
    }
    VkDescriptorPool GetHandle() const
    {
        return m_Pool;
    }
    operator VkDescriptorPool() const
    {
        return m_Pool;
    }
    operator bool() const
    {
        return m_Pool != VK_NULL_HANDLE;
    }

  private:
    LogicalDevice::Proxy m_Device{};
    VkDescriptorPool m_Pool = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
