#pragma once

#include "vkit/core/dimension.hpp"
#include "vkit/backend/instance.hpp"
#include "vkit/backend/device.hpp"
#include "vkit/core/vma.hpp"
#include "tkit/core/literals.hpp"

#ifndef VKIT_MAX_DESCRIPTOR_SETS
#    define VKIT_MAX_DESCRIPTOR_SETS 1000
#endif

#ifndef VKIT_MAX_STORAGE_BUFFER_DESCRIPTORS
#    define VKIT_MAX_STORAGE_BUFFER_DESCRIPTORS 1000
#endif

// This file handles the lifetime of global data the ONYX library needs, such as the Vulkan instance and device. To
// properly cleanup resources, ensure proper destruction ordering and avoid the extremely annoying static memory
// deallocation randomness, I use reference counting. In the terminate method, I just set the global references to
// nullptr to ensure the reference count goes to 0 just before the program ends, avoiding static mess

namespace TKit
{
class StackAllocator;
}

namespace VKit
{
class Window;
class DescriptorPool;
class DescriptorSetLayout;

using namespace TKit::Literals;

struct VKIT_API Core
{
    struct Specs
    {
        usize StackAllocatorMemory = 10_kb;
        Instance::Specs Instance;
    };

    static VkResult Initialize(const Specs &p_Specs) noexcept;
    static void Terminate() noexcept;

    static TKit::StackAllocator *GetStackAllocator() noexcept;

    static const TKit::Ref<Instance> &GetInstance() noexcept;
    static const TKit::Ref<Device> &GetDevice() noexcept;

    static VmaAllocator GetVulkanAllocator() noexcept;
};

} // namespace VKit
