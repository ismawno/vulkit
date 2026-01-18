#include "vkit/core/pch.hpp"
#include "vkit/execution/queue.hpp"

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
    case Queue_Count:
        return "Unknown";
    }
    return "Unknown";
}

Result<u64> Queue::UpdateCompletedTimeline()
{
    if (!m_Timeline)
        return Result<u64>::Error(Error_MissingFeature,
                                  "To query completed submissions of a queue it must have a "
                                  "timeline semaphore assigned with TakeTimelineSemaphoreOwnership()");

    const VkResult result = m_Device.Table->GetSemaphoreCounterValueKHR(m_Device, m_Timeline, &m_CompletedTimeline);
    if (result != VK_SUCCESS)
        return Result<u64>::Error(result);
    return m_CompletedTimeline;
}

void Queue::DestroyTimeline()
{
    if (m_Timeline)
    {
        m_Device.Table->DestroySemaphore(m_Device, m_Timeline, m_Device.AllocationCallbacks);
        m_Timeline = VK_NULL_HANDLE;
    }
}

Result<> Queue::Submit(TKit::Span<const VkSubmitInfo> p_Info, const VkFence p_Fence)
{
    if (m_TimelineCounter <= m_TimelineSubmissions)
        return Result<>::Error(Error_BadSynchronization,
                               "When submitting work from the submit queue methods, NextTimelineValue() must have been "
                               "called prior to that and the value returned must be used as a signal semaphore value "
                               "for the next submission (this last part is not checked)");

    const VkResult result = m_Device.Table->QueueSubmit(m_Queue, p_Info.GetSize(), p_Info.GetData(), p_Fence);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);

    m_TimelineSubmissions = m_TimelineCounter;
    return Result<>::Ok();
}

#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_synchronization2)
Result<> Queue::Submit2(const TKit::Span<const VkSubmitInfo2KHR> p_Info, const VkFence p_Fence)
{
    if (m_TimelineCounter <= m_TimelineSubmissions)
        return Result<>::Error(Error_BadSynchronization,
                               "When submitting work from the submit queue methods, NextTimelineValue() must have been "
                               "called prior to that and the value returned must be used as a signal semaphore value "
                               "for the next submission (this last part is not checked)");

    const VkResult result = m_Device.Table->QueueSubmit2KHR(m_Queue, p_Info.GetSize(), p_Info.GetData(), p_Fence);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);

    m_TimelineSubmissions = m_TimelineCounter;
    return Result<>::Ok();
}
#endif

Result<> Queue::WaitIdle() const
{
    const VkResult result = m_Device.Table->QueueWaitIdle(m_Queue);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);
    return Result<>::Ok();
}
} // namespace VKit
