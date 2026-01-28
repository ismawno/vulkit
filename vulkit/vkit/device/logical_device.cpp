#include "vkit/core/pch.hpp"
#include "vkit/device/logical_device.hpp"
#include "vkit/core/core.hpp"
#include "tkit/container/stack_array.hpp"

namespace VKit
{
LogicalDevice::Builder &LogicalDevice::Builder::RequireQueue(const QueueType type, const u32 count, f32 priority)
{
    return RequireQueue(m_PhysicalDevice->GetInfo().FamilyIndices[type], count, priority);
}
LogicalDevice::Builder &LogicalDevice::Builder::RequestQueue(const QueueType type, const u32 count, f32 priority)
{
    return RequestQueue(m_PhysicalDevice->GetInfo().FamilyIndices[type], count, priority);
}

LogicalDevice::Builder &LogicalDevice::Builder::RequireQueue(const u32 family, const u32 count, f32 priority)
{
    for (u32 i = 0; i < count; ++i)
        m_Priorities[family].RequiredPriorities.Append(priority);
    return *this;
}
LogicalDevice::Builder &LogicalDevice::Builder::RequestQueue(const u32 family, const u32 count, f32 priority)
{
    for (u32 i = 0; i < count; ++i)
        m_Priorities[family].RequestedPriorities.Append(priority);
    return *this;
}

Result<LogicalDevice> LogicalDevice::Builder::Build() const
{
    const Instance::Info &instanceInfo = m_Instance->GetInfo();
    PhysicalDevice::Info devInfo = m_PhysicalDevice->GetInfo();

    TKit::StackArray<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.Reserve(m_Priorities.GetSize());

    const TKit::TierArray<VkQueueFamilyProperties> &families = devInfo.QueueFamilies;
    TKit::StackArray<TKit::TierArray<f32>> finalPriorities;
    finalPriorities.Reserve(m_Priorities.GetSize());

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

        TKit::TierArray<f32> &fp = finalPriorities.Append();

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

    TKit::StackArray<const char *> enabledExtensions;
    enabledExtensions.Reserve(devInfo.EnabledExtensions.GetSize());
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
        return Result<LogicalDevice>::Error(
            Error_MissingExtension,
            "[VULKIT][L-DEVICE] The 'VK_KHR_get_physical_device_properties2' extension is not supported");
#endif

#if defined(VKIT_API_VERSION_1_1) || defined(VK_KHR_get_physical_device_properties2)
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pEnabledFeatures = nullptr;
    VkPhysicalDeviceFeatures2KHR fchain;
    if (v11 || prop2)
    {
        fchain = devInfo.EnabledFeatures.CreateChain(devInfo.ApiVersion);
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

    const Vulkan::InstanceTable *itable = instanceInfo.Table;

    VkDevice device;
    const VkResult result =
        itable->CreateDevice(*m_PhysicalDevice, &createInfo, instanceInfo.AllocationCallbacks, &device);
    if (result != VK_SUCCESS)
        return Result<LogicalDevice>::Error(result);

    TKit::TierAllocator *alloc = TKit::Memory::GetTier();
    Vulkan::DeviceTable *table =
        alloc->Create<Vulkan::DeviceTable>(Vulkan::DeviceTable::Create(device, *m_Instance->GetInfo().Table));

    Info info{};
    info.Instance = m_Instance;
    info.PhysicalDevice = m_PhysicalDevice;
    info.Table = table;

    ProxyDevice pdevice{device, m_Instance->GetInfo().AllocationCallbacks, table};
    const auto createQueue = [&](const u32 family, const u32 index) -> Result<Queue *> {
        for (u32 i = 0; i < info.QueuesPerType.GetSize(); ++i)
            if (index < info.QueuesPerType[i].GetSize() && info.QueuesPerType[i][index]->GetFamily() == family)
                return info.QueuesPerType[i][index];
        VkQueue q;
        table->GetDeviceQueue(pdevice, family, index, &q);
        return info.Queues.Append(alloc->Create<Queue>(pdevice, q, family));
    };

    const auto destroyQueues = [&] {
        for (VKit::Queue *queue : info.Queues)
            alloc->Destroy(queue);
    };

    for (u32 i = 0; i < queueCounts.GetSize(); ++i)
    {
        const u32 findex = devInfo.FamilyIndices[i];
        const u32 count = queueCounts[i];
        for (u32 j = 0; j < count; ++j)
        {
            const auto result = createQueue(findex, j);
            TKIT_RETURN_ON_ERROR(result, destroyQueues());
            info.QueuesPerType[i].Append(result.GetValue());
        }
    }

    return Result<LogicalDevice>::Ok(device, info);
}

void LogicalDevice::Destroy()
{
    if (m_Device)
    {
        TKit::TierAllocator *alloc = TKit::Memory::GetTier();
        for (VKit::Queue *q : m_Info.Queues)
        {
            q->DestroyTimeline();
            alloc->Destroy(q);
        }
        m_Info.Table->DestroyDevice(m_Device, m_Info.Instance->GetInfo().AllocationCallbacks);
        TKit::Memory::GetTier()->Destroy(m_Info.Table);
        m_Device = VK_NULL_HANDLE;
    }
}

Result<> LogicalDevice::WaitIdle(const ProxyDevice &device)
{
    const VkResult result = device.Table->DeviceWaitIdle(device);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);
    return Result<>::Ok();
}
Result<> LogicalDevice::WaitIdle() const
{
    return WaitIdle(*this);
}

#ifdef VK_KHR_surface
Result<PhysicalDevice::SwapChainSupportDetails> LogicalDevice::QuerySwapChainSupport(const VkSurfaceKHR surface) const
{
    return m_Info.PhysicalDevice->QuerySwapChainSupport(*m_Info.Instance, surface);
}
#endif

Result<VkFormat> LogicalDevice::FindSupportedFormat(TKit::Span<const VkFormat> candidates, const VkImageTiling tiling,
                                                    const VkFormatFeatureFlags features) const
{
    const Vulkan::InstanceTable *table = m_Info.Instance->GetInfo().Table;

    for (const VkFormat format : candidates)
    {
        VkFormatProperties props;
        table->GetPhysicalDeviceFormatProperties(*m_Info.PhysicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }
    return Result<VkFormat>::Error(Error_NoFormatSupported);
}

ProxyDevice LogicalDevice::CreateProxy() const
{
    ProxyDevice proxy;
    proxy.Device = m_Device;
    proxy.AllocationCallbacks = m_Info.Instance->GetInfo().AllocationCallbacks;
    proxy.Table = m_Info.Table;
    return proxy;
}

} // namespace VKit
