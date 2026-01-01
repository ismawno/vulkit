#pragma once

#include "vkit/vulkan/loader.hpp"

namespace VKit
{
struct ProxyDevice
{
    VkDevice Device = VK_NULL_HANDLE;
    const VkAllocationCallbacks *AllocationCallbacks = nullptr;
    const Vulkan::DeviceTable *Table = nullptr;

    operator VkDevice() const
    {
        return Device;
    }
    operator bool() const
    {
        return Device != VK_NULL_HANDLE;
    }
};
} // namespace VKit
