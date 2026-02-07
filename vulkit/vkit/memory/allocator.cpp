#include "vkit/vulkan/vulkan.hpp"
#define VMA_IMPLEMENTATION

#include "vkit/core/pch.hpp"
#include "vkit/memory/allocator.hpp"

namespace VKit
{
Result<VmaAllocator> CreateAllocator(const LogicalDevice &device, const AllocatorSpecs &specs)
{
    const Instance *instance = device.GetInfo().Instance;
    const PhysicalDevice *physicalDevice = device.GetInfo().PhysicalDevice;

    const Vulkan::InstanceTable *itable = instance->GetInfo().Table;
    const Vulkan::DeviceTable *dtable = device.GetInfo().Table;

    VmaVulkanFunctions functions{};
    functions.vkGetInstanceProcAddr = Vulkan::vkGetInstanceProcAddr;
    functions.vkGetDeviceProcAddr = itable->vkGetDeviceProcAddr;
    functions.vkGetPhysicalDeviceProperties = itable->vkGetPhysicalDeviceProperties;
    functions.vkGetPhysicalDeviceMemoryProperties = itable->vkGetPhysicalDeviceMemoryProperties;
    functions.vkAllocateMemory = dtable->vkAllocateMemory;
    functions.vkFreeMemory = dtable->vkFreeMemory;
    functions.vkMapMemory = dtable->vkMapMemory;
    functions.vkUnmapMemory = dtable->vkUnmapMemory;
    functions.vkFlushMappedMemoryRanges = dtable->vkFlushMappedMemoryRanges;
    functions.vkInvalidateMappedMemoryRanges = dtable->vkInvalidateMappedMemoryRanges;
    functions.vkBindBufferMemory = dtable->vkBindBufferMemory;
    functions.vkBindImageMemory = dtable->vkBindImageMemory;
    functions.vkGetBufferMemoryRequirements = dtable->vkGetBufferMemoryRequirements;
    functions.vkGetImageMemoryRequirements = dtable->vkGetImageMemoryRequirements;
    functions.vkCreateBuffer = dtable->vkCreateBuffer;
    functions.vkDestroyBuffer = dtable->vkDestroyBuffer;
    functions.vkCreateImage = dtable->vkCreateImage;
    functions.vkDestroyImage = dtable->vkDestroyImage;
    functions.vkCmdCopyBuffer = dtable->vkCmdCopyBuffer;
    const u32 version = physicalDevice->GetInfo().ApiVersion;
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
    if (version >= VKIT_MAKE_VERSION(0, 1, 1, 0))
    {
        functions.vkGetBufferMemoryRequirements2KHR = dtable->vkGetBufferMemoryRequirements2;
        functions.vkGetImageMemoryRequirements2KHR = dtable->vkGetImageMemoryRequirements2;
    }
    else
    {
        functions.vkGetBufferMemoryRequirements2KHR = dtable->vkGetBufferMemoryRequirements2KHR;
        functions.vkGetImageMemoryRequirements2KHR = dtable->vkGetImageMemoryRequirements2KHR;
    }
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
    if (version >= VKIT_MAKE_VERSION(0, 1, 1, 0))
    {
        functions.vkBindBufferMemory2KHR = dtable->vkBindBufferMemory2;
        functions.vkBindImageMemory2KHR = dtable->vkBindImageMemory2;
    }
    else
    {
        functions.vkBindBufferMemory2KHR = dtable->vkBindBufferMemory2KHR;
        functions.vkBindImageMemory2KHR = dtable->vkBindImageMemory2KHR;
    }
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
    if (version >= VKIT_MAKE_VERSION(0, 1, 1, 0))
        functions.vkGetPhysicalDeviceMemoryProperties2KHR = itable->vkGetPhysicalDeviceMemoryProperties2;
    else
        functions.vkGetPhysicalDeviceMemoryProperties2KHR = itable->vkGetPhysicalDeviceMemoryProperties2;

#endif
#if VMA_KHR_MAINTENANCE4 || VMA_VULKAN_VERSION >= 1003000
    if (version >= VKIT_MAKE_VERSION(0, 1, 3, 0))
    {
        functions.vkGetDeviceBufferMemoryRequirements = dtable->vkGetDeviceBufferMemoryRequirements;
        functions.vkGetDeviceImageMemoryRequirements = dtable->vkGetDeviceImageMemoryRequirements;
    }
    else
    {
        functions.vkGetDeviceBufferMemoryRequirements = dtable->vkGetDeviceBufferMemoryRequirementsKHR;
        functions.vkGetDeviceImageMemoryRequirements = dtable->vkGetDeviceImageMemoryRequirementsKHR;
    }
#endif

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = *physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = *instance;
    allocatorInfo.vulkanApiVersion = instance->GetInfo().ApplicationVersion;
    allocatorInfo.preferredLargeHeapBlockSize = specs.PreferredLargeHeapBlockSize;
    allocatorInfo.pAllocationCallbacks = instance->GetInfo().AllocationCallbacks;
    allocatorInfo.pDeviceMemoryCallbacks = specs.DeviceMemoryCallbacks;
    allocatorInfo.pHeapSizeLimit = specs.HeapSizeLimit;
#if VMA_EXTERNAL_MEMORY
    allocatorInfo.pTypeExternalMemoryHandleTypes = specs.ExternalMemoryHandleTypes;
#endif
    allocatorInfo.pVulkanFunctions = &functions;
    allocatorInfo.flags = specs.Flags;

    VmaAllocator allocator;
    VKIT_RETURN_IF_FAILED(vmaCreateAllocator(&allocatorInfo, &allocator), Result<VmaAllocator>);

    return allocator;
}
void DestroyAllocator(const VmaAllocator allocator)
{
    vmaDestroyAllocator(allocator);
}
} // namespace VKit
