#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/core/device.hpp"
#include "tkit/core/non_copyable.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/profiling/macros.hpp"
#include "tkit/profiling/vulkan.hpp"

#include <vulkan/vulkan.hpp>
#include <span>

namespace VKit
{
class VKIT_API DescriptorPool : public TKit::RefCounted<DescriptorPool>
{
    TKIT_NON_COPYABLE(DescriptorPool)
  public:
    struct Specs
    {
        u32 MaxSets;
        std::span<const VkDescriptorPoolSize> PoolSizes;
        VkDescriptorPoolCreateFlags PoolFlags = 0;
    };

    DescriptorPool(const Specs &p_Specs) noexcept;
    ~DescriptorPool() noexcept;

    VkDescriptorSet Allocate(VkDescriptorSetLayout p_Layout) const noexcept;

    void Deallocate(std::span<const VkDescriptorSet> p_Sets) const noexcept;
    void Deallocate(VkDescriptorSet p_Set) const noexcept;
    void Reset() noexcept;

  private:
    TKit::Ref<Device> m_Device;
    VkDescriptorPool m_Pool;

    // consider a pool per thread?
    mutable TKIT_PROFILE_DECLARE_MUTEX(std::mutex, m_Mutex);
};
} // namespace VKit