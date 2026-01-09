#include "vkit/core/pch.hpp"
#include "vkit/device/logical_device.hpp"

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
    TKit::FixedArray<u32, Queue_Count> queueCounts;
    for (u32 i = 0; i < Queue_Count; ++i)
        queueCounts[i] = 0;

    for (u32 index = 0; index < m_Priorities.GetSize(); ++index)
    {
        const QueuePriorities &priorities = m_Priorities[index];
        const VkQueueFamilyProperties &family = families[index];

        const u32 requiredCount = priorities.RequiredPriorities.GetSize();
        const u32 requestedCount = priorities.RequestedPriorities.GetSize();
        if (requiredCount > family.queueCount)
            return Result<LogicalDevice>::Error(
                Error_RejectedDevice,
                TKit::Format("The required queue count for the family index {} exceeds its queue count. {} >= {}",
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
            queueCounts[Queue_Graphics] += (devInfo.FamilyIndices[Queue_Graphics] == index) * count;
            queueCounts[Queue_Compute] += (devInfo.FamilyIndices[Queue_Compute] == index) * count;
            queueCounts[Queue_Transfer] += (devInfo.FamilyIndices[Queue_Transfer] == index) * count;
            queueCounts[Queue_Present] += (devInfo.FamilyIndices[Queue_Present] == index) * count;

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
    const bool prop2 = instanceInfo.Flags & InstanceFlag_Properties2Extension;
#ifndef VK_KHR_get_physical_device_properties2
    if (prop2)
        return Result<LogicalDevice>::Error(Error_MissingExtension,
                                            "The 'VK_KHR_get_physical_device_properties2' extension is not supported");
#endif

#if defined(VKIT_API_VERSION_1_1) || defined(VK_KHR_get_physical_device_properties2)
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pEnabledFeatures = nullptr;
    if (v11 || prop2)
    {
        const VkPhysicalDeviceFeatures2KHR fchain = devInfo.EnabledFeatures.CreateChain(devInfo.ApiVersion);
        createInfo.pNext = &fchain;
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

    VkDevice device;
    const VkResult result =
        itable->CreateDevice(*m_PhysicalDevice, &createInfo, instanceInfo.AllocationCallbacks, &device);
    if (result != VK_SUCCESS)
        return Result<LogicalDevice>::Error(result);

    const Vulkan::DeviceTable table = Vulkan::DeviceTable::Create(device, m_Instance->GetInfo().Table);

    info.Instance = *m_Instance;
    info.PhysicalDevice = *m_PhysicalDevice;
    info.Table = table;

    LogicalDevice ldevice{device, info};
    const auto createQueue = [&](const u32 p_Family, const u32 p_Index) -> Result<Queue *> {
        for (u32 i = 0; i < info.Queues.GetSize(); ++i)
            if (p_Index < info.Queues[i].GetSize() && info.Queues[i][p_Index]->GetFamily() == p_Family)
                return info.Queues[i][p_Index];
        VkQueue q;
        table.GetDeviceQueue(ldevice, p_Family, p_Index, &q);
        return new Queue(ldevice, q, p_Family);
    };

    for (u32 i = 0; i < queueCounts.GetSize(); ++i)
    {
        const u32 findex = devInfo.FamilyIndices[i];
        const u32 count = queueCounts[i];
        for (u32 j = 0; j < count; ++j)
        {
            const auto result = createQueue(findex, j);
            TKIT_RETURN_ON_ERROR(result);
            info.Queues[i].Append(result.GetValue());
        }
    }

    return Result<LogicalDevice>::Ok(device, info);
}

void LogicalDevice::Destroy()
{
    if (m_Device)
    {
        for (auto &queues : m_Info.Queues)
            for (auto &q : queues)
                if (q)
                {
                    q->DestroyTimeline();
                    delete q;
                    q = nullptr;
                }
        m_Info.Table.DestroyDevice(m_Device, m_Info.Instance.GetInfo().AllocationCallbacks);
        m_Device = VK_NULL_HANDLE;
    }
}

Result<> LogicalDevice::WaitIdle(const ProxyDevice &p_Device)
{
    const VkResult result = p_Device.Table->DeviceWaitIdle(p_Device);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);
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
    return Result<VkFormat>::Error(Error_NoFormatSupported);
}

ProxyDevice LogicalDevice::CreateProxy() const
{
    ProxyDevice proxy;
    proxy.Device = m_Device;
    proxy.AllocationCallbacks = m_Info.Instance.GetInfo().AllocationCallbacks;
    proxy.Table = &m_Info.Table;
    return proxy;
}

} // namespace VKit
