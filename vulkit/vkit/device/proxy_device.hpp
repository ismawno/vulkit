#pragma once

#include "vkit/vulkan/loader.hpp"

namespace VKit
{
#ifdef VK_EXT_debug_utils
template <typename Handle>
VKIT_NO_DISCARD Result<> SetObjectName(const VkDevice device, const Vulkan::DeviceTable *table, const Handle handle,
                                       const VkObjectType objType, const char *name)
{
    VkDebugUtilsObjectNameInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType = objType;
    info.pObjectName = name;
    info.objectHandle = reinterpret_cast<u64>(handle);
    VKIT_RETURN_IF_FAILED(table->SetDebugUtilsObjectNameEXT(device, &info), Result<>);
    return Result<>::Ok();
}
#endif
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

#ifdef VK_EXT_debug_utils
    template <typename Handle>
    VKIT_NO_DISCARD Result<> SetObjectName(const Handle handle, const VkObjectType objType, const char *name) const
    {
        return VKit::SetObjectName(Device, Table, handle, objType, name);
    }
#endif
};
} // namespace VKit
