#include "vkit/core/pch.hpp"
#include "vkit/execution/queue.hpp"
#include "vkit/device/logical_device.hpp"

namespace VKit
{
const char *ToString(const QueueType p_Type)
{
    switch (p_Type)
    {
    case Queue_Graphics:
        return "Graphics";
    case Queue_Compute:
        return "Compute";
    case Queue_Transfer:
        return "Transfer";
    case Queue_Present:
        return "Present";
    }
    return "Unknown";
}

#ifdef VK_KHR_timeline_semaphore
Result<Queue> Queue::Create(const LogicalDevice &p_Device, const VkQueue p_Queue, const u32 p_Family, const u32 p_Index)
{
    const LogicalDevice::Info &info = p_Device.GetInfo();
    const ProxyDevice proxy = p_Device;
    const PhysicalDevice &pdev = info.PhysicalDevice;
#    ifdef VKIT_API_VERSION_1_2
    PhysicalDevice::Features features{};
    features.Vulkan12.timelineSemaphore = VK_TRUE;
    const bool timelineEnabled = pdev.AreFeaturesEnabled(features);
#    else
    VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timelineSemaphore;
    timelineSemaphore.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;
    timelineSemaphore.timelineSemaphore = VK_TRUE;
    VkPhysicalDeviceFeatures2KHR fchain{};
    fchain.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
    fchain.pNext = &timelineSemaphore;

    info.Instance.GetInfo().Table.GetPhysicalDeviceFeatures2KHR(pdev, &fchain);
    const bool timelineEnabled = static_cast<bool>(timelineSemaphore.timelineSemaphore);
#    endif
    if (!timelineEnabled)
        return Queue{p_Device, p_Queue, p_Family, p_Index};

    VkSemaphoreTypeCreateInfoKHR tinfo{};
    tinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR;
    tinfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
    tinfo.initialValue = 0;

    VkSemaphoreCreateInfo cinfo{};
    cinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    cinfo.pNext = &tinfo;

    VkSemaphore timeline;
    const VkResult result = info.Table.CreateSemaphore(p_Device, &cinfo, proxy.AllocationCallbacks, &timeline);
    if (result != VK_SUCCESS)
        return Result<Queue>::Error(result, "Failed to create timeline semaphore");

    return Queue{p_Device, p_Queue, p_Family, p_Index, timeline};
}

Result<u64> Queue::GetCompletedSubmissionCount() const
{
    if (!m_Timeline)
        return Result<u64>::Error(
            VK_ERROR_FEATURE_NOT_PRESENT,
            "To query completed submissions of a queue, the VK_KHR_timeline_semaphore feature must be enabled");
    u64 count;
    const VkResult result = m_Device.Table->GetSemaphoreCounterValueKHR(m_Device, m_Timeline, &count);
    if (result != VK_SUCCESS)
        return Result<u64>::Error(result, "Failed to query semaphore counter value");

    return count;
}
Result<u64> Queue::GetPendingSubmissionCount() const
{
    const auto result = GetCompletedSubmissionCount();
    TKIT_RETURN_ON_ERROR(result);
    return m_SubmissionCount - result.GetValue();
}

#else
Result<Queue> Queue::Create(const VkQueue p_Queue, const u32 p_Family, const u32 p_Index)
{
    return Queue{p_Queue, p_Family, p_Index};
}
#endif

} // namespace VKit
