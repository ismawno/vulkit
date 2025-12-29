#include "vkit/core/pch.hpp"
#include "vkit/vulkan/logical_device.hpp"

namespace VKit
{
LogicalDevice::Builder &LogicalDevice::Builder::RequireQueue(const QueueType p_Type, const u32 p_Count, f32 p_Priority)
{
    return RequireQueue(m_PhysicalDevice->GetInfo().FamilyIndices[p_Type], p_Count, p_Priority);
}
LogicalDevice::Builder &LogicalDevice::Builder::RequestQueue(const QueueType p_Type, const u32 p_Count, f32 p_Priority)
{
    return RequestQueue(m_PhysicalDevice->GetInfo().FamilyIndices[p_Type], p_Count, p_Priority);
}

LogicalDevice::Builder &LogicalDevice::Builder::RequireQueue(const u32 p_Family, const u32 p_Count, f32 p_Priority)
{
    for (u32 i = 0; i < p_Count; ++i)
        m_Priorities[p_Family].RequiredPriorities.Append(p_Priority);
    return *this;
}
LogicalDevice::Builder &LogicalDevice::Builder::RequestQueue(const u32 p_Family, const u32 p_Count, f32 p_Priority)
{
    for (u32 i = 0; i < p_Count; ++i)
        m_Priorities[p_Family].RequestedPriorities.Append(p_Priority);
    return *this;
}

Result<LogicalDevice> LogicalDevice::Builder::Build() const
{
    const Instance::Info &instanceInfo = m_Instance->GetInfo();
    PhysicalDevice::Info devInfo = m_PhysicalDevice->GetInfo();

    TKit::Array8<VkDeviceQueueCreateInfo> queueCreateInfos;
    const TKit::Array8<VkQueueFamilyProperties> &families = devInfo.QueueFamilies;

    TKit::Array8<TKit::Array32<f32>> finalPriorities;

    Info info{};
    for (u32 i = 0; i < 4; ++i)
        info.QueueCounts[i] = 0;

    for (u32 index = 0; index < m_Priorities.GetSize(); ++index)
    {
        const QueuePriorities &priorities = m_Priorities[index];
        const VkQueueFamilyProperties &family = families[index];

        const u32 requiredCount = priorities.RequiredPriorities.GetSize();
        const u32 requestedCount = priorities.RequestedPriorities.GetSize();
        if (requiredCount > family.queueCount)
            return Result<LogicalDevice>::Error(
                VKIT_FORMAT_ERROR(VK_ERROR_FEATURE_NOT_PRESENT,
                                  "The required queue count for the family index {} exceeds its queue count. {} > {}",
                                  index, requiredCount, family.queueCount));

        TKit::Array32<f32> &fp = finalPriorities.Append();

        for (u32 i = 0; i < requiredCount; ++i)
            fp.Append(priorities.RequiredPriorities[i]);

        for (u32 i = 0; i < requestedCount; ++i)
            fp.Append(priorities.RequestedPriorities[i]);

        const u32 totalCount = requiredCount + requestedCount;
        const u32 count = std::min(family.queueCount, totalCount);

        TKIT_LOG_WARNING_IF(count < totalCount,
                            "[VULKIT] Not all requested queues could be created for the family index {} as the "
                            "combined queue count of {} surpasses the family's queue count of {}",
                            index, totalCount, family.queueCount);

        if (!fp.IsEmpty())
        {
            info.QueueCounts[Queue_Graphics] += (devInfo.FamilyIndices[Queue_Graphics] == index) * count;
            info.QueueCounts[Queue_Compute] += (devInfo.FamilyIndices[Queue_Compute] == index) * count;
            info.QueueCounts[Queue_Transfer] += (devInfo.FamilyIndices[Queue_Transfer] == index) * count;
            info.QueueCounts[Queue_Present] += (devInfo.FamilyIndices[Queue_Present] == index) * count;

            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = index;
            queueCreateInfo.queueCount = count;
            queueCreateInfo.pQueuePriorities = fp.GetData();
            queueCreateInfos.Append(queueCreateInfo);
        }
    }

    TKit::Array256<const char *> enabledExtensions;
    for (const std::string &extension : devInfo.EnabledExtensions)
        enabledExtensions.Append(extension.c_str());

#ifdef VKIT_API_VERSION_1_1
    const bool v11 = devInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 1, 0);
#else
    const bool v11 = false;
#endif
    const bool prop2 = instanceInfo.Flags & Instance::Flag_Properties2Extension;
#ifndef VK_KHR_get_physical_device_properties2
    if (prop2)
        return Result<LogicalDevice>::Error(
            VK_ERROR_EXTENSION_NOT_PRESENT, "The 'VK_KHR_get_physical_device_properties2' extension is not supported");
#endif

#if defined(VKIT_API_VERSION_1_1) || defined(VK_KHR_get_physical_device_properties2)
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pEnabledFeatures = nullptr;

#    ifdef VKIT_API_VERSION_1_1
    VkPhysicalDeviceFeatures2 featuresChain{};
#    else
    VkPhysicalDeviceFeatures2KHR featuresChain{};
#    endif

    if (v11 || prop2)
    {
#    ifdef VKIT_API_VERSION_1_1
        featuresChain.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
#    else
        featuresChain.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
#    endif
        featuresChain.features = devInfo.EnabledFeatures.Core;

        const auto fillChain = [&] {
#    ifdef VKIT_API_VERSION_1_2
            if (devInfo.ApiVersion >= VKIT_API_VERSION_1_2)
            {
                featuresChain.pNext = &devInfo.EnabledFeatures.Vulkan11;
                devInfo.EnabledFeatures.Vulkan11.pNext = &devInfo.EnabledFeatures.Vulkan12;
            }
            else
            {
                featuresChain.pNext = devInfo.EnabledFeatures.Next;
                return;
            }
#    else
            featuresChain.pNext = devInfo.EnabledFeatures.Next;
#    endif

#    ifdef VKIT_API_VERSION_1_3
            if (devInfo.ApiVersion >= VKIT_API_VERSION_1_3)
                devInfo.EnabledFeatures.Vulkan12.pNext = &devInfo.EnabledFeatures.Vulkan13;
            else
            {
                devInfo.EnabledFeatures.Vulkan12.pNext = devInfo.EnabledFeatures.Next;
                return;
            }
#    endif
#    ifdef VKIT_API_VERSION_1_4
            if (devInfo.ApiVersion >= VKIT_API_VERSION_1_4)
                devInfo.EnabledFeatures.Vulkan13.pNext = &devInfo.EnabledFeatures.Vulkan14;
            else
            {
                devInfo.EnabledFeatures.Vulkan13.pNext = devInfo.EnabledFeatures.Next;
                return;
            }
#    endif
        };

        fillChain();
        createInfo.pNext = &featuresChain;
    }
    else
    {
        createInfo.pEnabledFeatures = &devInfo.EnabledFeatures.Core;
        createInfo.pNext = devInfo.EnabledFeatures.Next;
    }
#else
    createInfo.pEnabledFeatures = &devInfo.EnabledFeatures.Core;
    createInfo.pNext = devInfo.EnabledFeatures.Next;
#endif

    createInfo.queueCreateInfoCount = queueCreateInfos.GetSize();
    createInfo.pQueueCreateInfos = queueCreateInfos.GetData();
    createInfo.enabledExtensionCount = enabledExtensions.GetSize();
    createInfo.ppEnabledExtensionNames = enabledExtensions.GetData();
    createInfo.enabledLayerCount = instanceInfo.EnabledLayers.GetSize();
    createInfo.ppEnabledLayerNames = instanceInfo.EnabledLayers.GetData();

    const Vulkan::InstanceTable *itable = &instanceInfo.Table;

    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(itable, vkCreateDevice, Result<LogicalDevice>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(itable, vkGetPhysicalDeviceFormatProperties, Result<LogicalDevice>);

    VkDevice device;
    const VkResult result =
        itable->CreateDevice(*m_PhysicalDevice, &createInfo, instanceInfo.AllocationCallbacks, &device);
    if (result != VK_SUCCESS)
        return Result<LogicalDevice>::Error(result, "Failed to create the logical device");

    const Vulkan::DeviceTable table = Vulkan::DeviceTable::Create(device, m_Instance->GetInfo().Table);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN((&table), vkDestroyDevice, Result<LogicalDevice>);

    if (!table.vkGetDeviceQueue)
    {
        table.DestroyDevice(device, instanceInfo.AllocationCallbacks);
        return Result<LogicalDevice>::Error(VK_ERROR_INCOMPATIBLE_DRIVER, "Failed to load Vulkan function: "
                                                                                   "vkGetDeviceQueue");
    }

    info.Instance = *m_Instance;
    info.PhysicalDevice = *m_PhysicalDevice;
    info.Table = table;
    return Result<LogicalDevice>::Ok(device, info);
}

void LogicalDevice::Destroy()
{
    if (m_Device)
    {
        m_Info.Table.DestroyDevice(m_Device, m_Info.Instance.GetInfo().AllocationCallbacks);
        m_Device = VK_NULL_HANDLE;
    }
}

Result<> LogicalDevice::WaitIdle(const Proxy &p_Device)
{
    const VkResult result = p_Device.Table->DeviceWaitIdle(p_Device);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to wait for device");
    return Result<>::Ok();
}
Result<> LogicalDevice::WaitIdle() const
{
    return WaitIdle(*this);
}

#ifdef VK_KHR_surface
Result<PhysicalDevice::SwapChainSupportDetails> LogicalDevice::QuerySwapChainSupport(const VkSurfaceKHR p_Surface) const
{
    return m_Info.PhysicalDevice.QuerySwapChainSupport(m_Info.Instance, p_Surface);
}
#endif

Result<VkFormat> LogicalDevice::FindSupportedFormat(TKit::Span<const VkFormat> p_Candidates,
                                                    const VkImageTiling p_Tiling,
                                                    const VkFormatFeatureFlags p_Features) const
{
    const Vulkan::InstanceTable *table = &m_Info.Instance.GetInfo().Table;

    for (const VkFormat format : p_Candidates)
    {
        VkFormatProperties props;
        table->GetPhysicalDeviceFormatProperties(m_Info.PhysicalDevice, format, &props);

        if (p_Tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & p_Features) == p_Features)
            return format;
        if (p_Tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & p_Features) == p_Features)
            return format;
    }
    return Result<VkFormat>::Error(VK_ERROR_FORMAT_NOT_SUPPORTED, "No supported format found");
}

LogicalDevice::Proxy LogicalDevice::CreateProxy() const
{
    Proxy proxy;
    proxy.Device = m_Device;
    proxy.AllocationCallbacks = m_Info.Instance.GetInfo().AllocationCallbacks;
    proxy.Table = &m_Info.Table;
    return proxy;
}

Result<VkQueue> LogicalDevice::GetQueue(const QueueType p_Type, const u32 p_QueueIndex) const
{
    const PhysicalDevice::Info &info = m_Info.PhysicalDevice.GetInfo();
    return GetQueue(info.FamilyIndices[p_Type], p_QueueIndex);
}

Result<VkQueue> LogicalDevice::GetQueue(const u32 p_FamilyIndex, const u32 p_QueueIndex) const
{
    const PhysicalDevice::Info &info = m_Info.PhysicalDevice.GetInfo();

    bool found = false;
    for (u32 i = 0; i < 4; ++i)
    {
        const bool known = info.FamilyIndices[i] == p_FamilyIndex;
        found |= known;
        if (known && p_QueueIndex >= m_Info.QueueCounts[i])
            return Result<VkQueue>::Error(
                VK_ERROR_FEATURE_NOT_PRESENT,
                "Failed to retrieve queue. Index exceeds queue count for the given family index. "
                "Try to request more queues of this family when creating the logical device");
    }
    if (!found)
        return Result<VkQueue>::Error(VK_ERROR_UNKNOWN, "Unknown family index");

    VkQueue queue;
    m_Info.Table.GetDeviceQueue(m_Device, p_FamilyIndex, p_QueueIndex, &queue);
    return queue;
}

} // namespace VKit
