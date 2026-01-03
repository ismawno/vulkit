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

#if defined(VKIT_API_VERSION_1_2) || defined(VK_KHR_timeline_semaphore)
Result<Queue> Queue::Create(const LogicalDevice &p_Device, const VkQueue p_Queue, const u32 p_Family)
{
    const LogicalDevice::Info &info = p_Device.GetInfo();
    const ProxyDevice proxy = p_Device;
    const PhysicalDevice &pdev = info.PhysicalDevice;
#    ifdef VKIT_API_VERSION_1_2
    DeviceFeatures features{};
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
        return Queue{p_Device, p_Queue, p_Family};

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

    return Queue{p_Device, p_Queue, p_Family, timeline};
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

Result<u64> Queue::Submit(VkSubmitInfo p_Info, const VkFence p_Fence)
{
    const u64 signal = m_SubmissionCount + 1;
    if (m_Timeline)
    {
        VkTimelineSemaphoreSubmitInfoKHR timeline{};
        timeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR;
        timeline.signalSemaphoreValueCount = 1;
        timeline.pSignalSemaphoreValues = &signal;

        const void *next = p_Info.pNext;
        timeline.pNext = next;
        p_Info.pNext = &timeline;

        TKit::Array<VkSemaphore, MaxQueueSubmissions> signalSemaphores{};
        const u32 count = p_Info.pSignalSemaphores ? p_Info.signalSemaphoreCount : 0;
        for (u32 i = 0; i < count; ++i)
            signalSemaphores.Append(p_Info.pSignalSemaphores[i]);

        signalSemaphores.Append(m_Timeline);
        p_Info.pSignalSemaphores = signalSemaphores.GetData();
        p_Info.signalSemaphoreCount = signalSemaphores.GetSize();

        const VkResult result = m_Device.Table->QueueSubmit(m_Queue, 1, &p_Info, p_Fence);
        if (result != VK_SUCCESS)
            return Result<>::Error(result, "Failed to submit to queue");

        m_SubmissionCount = signal;
        return signal;
    }

    const VkResult result = m_Device.Table->QueueSubmit(m_Queue, 1, &p_Info, p_Fence);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to submit to queue");

    m_SubmissionCount = signal;
    return signal;
}

#else
Result<Queue> Queue::Create(const VkQueue p_Queue, const u32 p_Family)
{
    return Queue{p_Queue, p_Family};
}
Result<u64> Queue::Submit(const VkSubmitInfo &p_Info, const VkFence p_Fence)
{
    const VkResult result = m_Device.Table->QueueSubmit(m_Queue, 1, &p_Info, p_Fence);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to submit to queue");

    return ++m_SubmissionCount;
}
#endif

Result<TKit::Array<u64, MaxQueueSubmissions>> Queue::Submit(const TKit::Span<const VkSubmitInfo> p_Info,
                                                            const VkFence p_Fence)
{
    u64 signal = m_SubmissionCount;
    TKit::Array<u64, MaxQueueSubmissions> signals{};
#if defined(VKIT_API_VERSION_1_2) || defined(VK_KHR_timeline_semaphore)
    if (m_Timeline)
    {
        TKit::Array<VkSubmitInfo, MaxQueueSubmissions> infos{};
        TKit::Array<TKit::Array8<VkSemaphore>, MaxQueueSubmissions> signalSemaphores{};
        TKit::Array<VkTimelineSemaphoreSubmitInfoKHR, MaxQueueSubmissions> timelines{};
        for (VkSubmitInfo info : p_Info)
        {
            VkTimelineSemaphoreSubmitInfoKHR &timeline = timelines.Append();
            timeline = {};
            timeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR;
            timeline.signalSemaphoreValueCount = 1;
            timeline.pSignalSemaphoreValues = &signals.Append(++signal);

            const void *next = info.pNext;
            timeline.pNext = next;
            info.pNext = &timeline;

            TKit::Array8<VkSemaphore> &semaphores = signalSemaphores.Append();
            const u32 count = info.pSignalSemaphores ? info.signalSemaphoreCount : 0;
            for (u32 i = 0; i < count; ++i)
                semaphores.Append(info.pSignalSemaphores[i]);

            semaphores.Append(m_Timeline);
            info.pSignalSemaphores = semaphores.GetData();
            info.signalSemaphoreCount = semaphores.GetSize();
            infos.Append(info);
        }
        const VkResult result = m_Device.Table->QueueSubmit(m_Queue, infos.GetSize(), infos.GetData(), p_Fence);
        if (result != VK_SUCCESS)
            return Result<>::Error(result, "Failed to submit to queue");
        m_SubmissionCount = signal;
        return signals;
    }
    else
    {
        for (u32 i = 0; i < p_Info.GetSize(); ++i)
            signals[i] = ++signal;

        const VkResult result = m_Device.Table->QueueSubmit(m_Queue, p_Info.GetSize(), p_Info.GetData(), p_Fence);
        if (result != VK_SUCCESS)
            return Result<>::Error(result, "Failed to submit to queue");
        m_SubmissionCount = signal;
        return signals;
    }
#else
    for (u32 i = 0; i < p_Info.GetSize(); ++i)
        signals[i] = ++signal;
    const VkResult result = m_Device.Table->QueueSubmit(m_Queue, p_Info.GetSize(), p_Info.GetData(), p_Fence);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to submit to queue");
    m_SubmissionCount = signal;
    return signals;
#endif
}

#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_synchronization2)
Result<u64> Queue::Submit(VkSubmitInfo2KHR p_Info, const VkFence p_Fence)
{
    const u64 signal = m_SubmissionCount + 1;
    if (m_Timeline)
    {
        VkSemaphoreSubmitInfoKHR timeline{};

        timeline.semaphore = m_Timeline;
        timeline.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
        timeline.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
        timeline.value = signal;

        TKit::Array<VkSemaphoreSubmitInfoKHR, MaxQueueSubmissions> signalSemaphores{};
        const u32 count = p_Info.pSignalSemaphoreInfos ? p_Info.signalSemaphoreInfoCount : 0;
        for (u32 i = 0; i < count; ++i)
            signalSemaphores.Append(p_Info.pSignalSemaphoreInfos[i]);

        signalSemaphores.Append(timeline);
        p_Info.pSignalSemaphoreInfos = signalSemaphores.GetData();
        p_Info.signalSemaphoreInfoCount = signalSemaphores.GetSize();

        const VkResult result = m_Device.Table->QueueSubmit2KHR(m_Queue, 1, &p_Info, p_Fence);
        if (result != VK_SUCCESS)
            return Result<>::Error(result, "Failed to submit to queue");
        m_SubmissionCount = signal;
        return Result<>::Ok();
    }

    const VkResult result = m_Device.Table->QueueSubmit2KHR(m_Queue, 1, &p_Info, p_Fence);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to submit to queue");

    m_SubmissionCount = signal;
    return signal;
}
Result<TKit::Array<u64, MaxQueueSubmissions>> Queue::Submit(const TKit::Span<const VkSubmitInfo2KHR> p_Info,
                                                            const VkFence p_Fence)
{
    u64 signal = m_SubmissionCount;
    TKit::Array<u64, MaxQueueSubmissions> signals{};
    if (m_Timeline)
    {
        TKit::Array<VkSubmitInfo2KHR, MaxQueueSubmissions> infos{};
        TKit::Array<TKit::Array8<VkSemaphoreSubmitInfoKHR>, MaxQueueSubmissions> signalSemaphores{};
        for (VkSubmitInfo2KHR info : p_Info)
        {
            VkSemaphoreSubmitInfoKHR timeline{};
            timeline.semaphore = m_Timeline;
            timeline.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
            timeline.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
            timeline.value = ++signal;
            signals.Append(signal);

            TKit::Array8<VkSemaphoreSubmitInfoKHR> &signal = signalSemaphores.Append();
            const u32 count = info.pSignalSemaphoreInfos ? info.signalSemaphoreInfoCount : 0;
            for (u32 i = 0; i < count; ++i)
                signal.Append(info.pSignalSemaphoreInfos[i]);

            signal.Append(timeline);
            info.pSignalSemaphoreInfos = signal.GetData();
            info.signalSemaphoreInfoCount = signal.GetSize();
            infos.Append(info);
        }
        const VkResult result = m_Device.Table->QueueSubmit2(m_Queue, infos.GetSize(), infos.GetData(), p_Fence);
        if (result != VK_SUCCESS)
            return Result<>::Error(result, "Failed to submit to queue");

        m_SubmissionCount = signal;
        return Result<>::Ok();
    }
    else
    {
        for (u32 i = 0; i < p_Info.GetSize(); ++i)
            signals[i] = ++signal;

        const VkResult result = m_Device.Table->QueueSubmit2(m_Queue, p_Info.GetSize(), p_Info.GetData(), p_Fence);
        if (result != VK_SUCCESS)
            return Result<>::Error(result, "Failed to submit to queue");
        m_SubmissionCount = signal;
        return signals;
    }
}
#endif

Result<> Queue::WaitIdle() const
{
    const VkResult result = m_Device.Table->QueueWaitIdle(m_Queue);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to wait for queue");
    return Result<>::Ok();
}
} // namespace VKit
