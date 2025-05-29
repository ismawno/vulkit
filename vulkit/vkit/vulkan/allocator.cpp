#define VMA_IMPLEMENTATION

#include "vkit/core/pch.hpp"
#include "vkit/vulkan/allocator.hpp"

namespace VKit
{
Result<VmaAllocator> CreateAllocator(const LogicalDevice &p_Device, const AllocatorSpecs &p_Specs) noexcept
{
    const Instance &instance = p_Device.GetInstance();
    const PhysicalDevice &physicalDevice = p_Device.GetPhysicalDevice();

    const Vulkan::InstanceTable &itable = instance.GetInfo().Table;
    const Vulkan::DeviceTable &dtable = p_Device.GetTable();

    VmaVulkanFunctions functions{};
    functions.vkGetInstanceProcAddr = Vulkan::vkGetInstanceProcAddr;
    functions.vkGetDeviceProcAddr = instance.GetInfo().Table.vkGetDeviceProcAddr;
    functions.vkGetPhysicalDeviceProperties = itable.vkGetPhysicalDeviceProperties;
    functions.vkGetPhysicalDeviceMemoryProperties = itable.vkGetPhysicalDeviceMemoryProperties;
    functions.vkAllocateMemory = dtable.vkAllocateMemory;
    functions.vkFreeMemory = dtable.vkFreeMemory;
    functions.vkMapMemory = dtable.vkMapMemory;
    functions.vkUnmapMemory = dtable.vkUnmapMemory;
    functions.vkFlushMappedMemoryRanges = dtable.vkFlushMappedMemoryRanges;
    functions.vkInvalidateMappedMemoryRanges = dtable.vkInvalidateMappedMemoryRanges;
    functions.vkBindBufferMemory = dtable.vkBindBufferMemory;
    functions.vkBindImageMemory = dtable.vkBindImageMemory;
    functions.vkGetBufferMemoryRequirements = dtable.vkGetBufferMemoryRequirements;
    functions.vkGetImageMemoryRequirements = dtable.vkGetImageMemoryRequirements;
    functions.vkCreateBuffer = dtable.vkCreateBuffer;
    functions.vkDestroyBuffer = dtable.vkDestroyBuffer;
    functions.vkCreateImage = dtable.vkCreateImage;
    functions.vkDestroyImage = dtable.vkDestroyImage;
    functions.vkCmdCopyBuffer = dtable.vkCmdCopyBuffer;
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
    functions.vkGetBufferMemoryRequirements2KHR = dtable.vkGetBufferMemoryRequirements2KHR;
    functions.vkGetImageMemoryRequirements2KHR = dtable.vkGetImageMemoryRequirements2KHR;
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
    functions.vkBindBufferMemory2KHR = dtable.vkBindBufferMemory2KHR;
    functions.vkBindImageMemory2KHR = dtable.vkBindImageMemory2KHR;
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
    functions.vkGetPhysicalDeviceMemoryProperties2KHR = itable.vkGetPhysicalDeviceMemoryProperties2KHR;
#endif
#if VMA_KHR_MAINTENANCE4 || VMA_VULKAN_VERSION >= 1003000
    functions.vkGetDeviceBufferMemoryRequirements = dtable.vkGetDeviceBufferMemoryRequirementsKHR;
    functions.vkGetDeviceImageMemoryRequirements = dtable.vkGetDeviceImageMemoryRequirementsKHR;
#endif

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = p_Device;
    allocatorInfo.instance = instance;
    allocatorInfo.vulkanApiVersion = instance.GetInfo().ApplicationVersion;
    allocatorInfo.preferredLargeHeapBlockSize = p_Specs.PreferredLargeHeapBlockSize;
    allocatorInfo.pAllocationCallbacks = instance.GetInfo().AllocationCallbacks;
    allocatorInfo.pDeviceMemoryCallbacks = p_Specs.DeviceMemoryCallbacks;
    allocatorInfo.pHeapSizeLimit = p_Specs.HeapSizeLimit;
#if VMA_EXTERNAL_MEMORY
    allocatorInfo.pTypeExternalMemoryHandleTypes = p_Specs.ExternalMemoryHandleTypes;
#endif
    allocatorInfo.pVulkanFunctions = &functions;
    allocatorInfo.flags = p_Specs.Flags;

    VmaAllocator allocator;
    const VkResult result = vmaCreateAllocator(&allocatorInfo, &allocator);
    if (result != VK_SUCCESS)
        return Result<VmaAllocator>::Error(result, "Failed to create VMA allocator");

    return Result<VmaAllocator>::Ok(allocator);
}
void DestroyAllocator(const VmaAllocator p_Allocator) noexcept
{
    vmaDestroyAllocator(p_Allocator);
}
} // namespace VKit
