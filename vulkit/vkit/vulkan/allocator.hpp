#pragma once

#include "vkit/vulkan/logical_device.hpp"
#define VMA_NULLABLE
#define VMA_NOT_NULL
TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_GCC_WARNING_IGNORE("-Wunused-parameter")
TKIT_GCC_WARNING_IGNORE("-Wunused-variable")
TKIT_GCC_WARNING_IGNORE("-Wunused-function")
TKIT_GCC_WARNING_IGNORE("-Wmissing-field-initializers")
TKIT_CLANG_WARNING_IGNORE("-Wunused-parameter")
TKIT_CLANG_WARNING_IGNORE("-Wunused-variable")
TKIT_CLANG_WARNING_IGNORE("-Wunused-function")
TKIT_CLANG_WARNING_IGNORE("-Wmissing-field-initializers")
TKIT_MSVC_WARNING_IGNORE(4100)
TKIT_MSVC_WARNING_IGNORE(4189)
TKIT_MSVC_WARNING_IGNORE(4505)
TKIT_MSVC_WARNING_IGNORE(4351)
TKIT_MSVC_WARNING_IGNORE(4127)
TKIT_MSVC_WARNING_IGNORE(4324)
#include <vk_mem_alloc.h>
TKIT_COMPILER_WARNING_IGNORE_POP()

namespace VKit
{
struct AllocatorSpecs
{
    VkDeviceSize PreferredLargeHeapBlockSize = 0;
    const VmaDeviceMemoryCallbacks *DeviceMemoryCallbacks = nullptr;
    const VkDeviceSize *HeapSizeLimit = nullptr;
#if VMA_EXTERNAL_MEMORY
    const VkExternalMemoryHandleTypeFlagsKHR *ExternalMemoryHandleTypes = nullptr;
#endif
    VmaAllocatorCreateFlags Flags = 0;
};

Result<VmaAllocator> CreateAllocator(const LogicalDevice &p_Device, const AllocatorSpecs &p_Specs) noexcept;
void DestroyAllocator(VmaAllocator p_Allocator) noexcept;
} // namespace VKit