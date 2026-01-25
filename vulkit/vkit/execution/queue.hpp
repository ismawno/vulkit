#pragma once

#include "vkit/core/alias.hpp"
#include "vkit/device/proxy_device.hpp"
#include "tkit/container/span.hpp"

namespace VKit
{
enum QueueType : u32
{
    Queue_Graphics,
    Queue_Compute,
    Queue_Transfer,
    Queue_Present,
    Queue_Count,
};

const char *ToString(QueueType type);

class LogicalDevice;

// not meant to be created by user, only by logical device.
// timeline semaphore must be explicitly submitted. it wont do it for you. the only convenience is to have a timeline
// handle and counter per queue
class Queue
{
  public:
    Queue() = default;

    Queue(const ProxyDevice &device, const VkQueue queue, const u32 family)
        : m_Device(device), m_Queue(queue), m_Family(family)
    {
    }

    VKIT_NO_DISCARD Result<> Submit(TKit::Span<const VkSubmitInfo> info, VkFence fence = VK_NULL_HANDLE);
#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_synchronization2)
    VKIT_NO_DISCARD Result<> Submit2(TKit::Span<const VkSubmitInfo2KHR> info, VkFence fence = VK_NULL_HANDLE);
#endif

    VKIT_NO_DISCARD Result<> WaitIdle() const;
    const ProxyDevice &GetDevice() const
    {
        return m_Device;
    }

    VkQueue GetHandle() const
    {
        return m_Queue;
    }

    u64 NextTimelineValue()
    {
        return ++m_TimelineCounter;
    }
    void RevokeUnsubmittedTimelineValues()
    {
        m_TimelineCounter = m_TimelineSubmissions;
    }

    u64 GetTimelineValue() const
    {
        return m_TimelineCounter;
    }

    u32 GetFamily() const
    {
        return m_Family;
    }

    operator VkQueue() const
    {
        return m_Queue;
    }
    operator bool() const
    {
        return m_Queue != VK_NULL_HANDLE;
    }

    void DestroyTimeline();

    void TakeTimelineSemaphoreOwnership(const VkSemaphore timeline, const u64 initialSubmissionCount = 0)
    {
        TKIT_ASSERT(!m_Timeline, "[ONYX] The current queue already has a timeline semaphore. Ensure the previous one "
                                 "is destroyed with DestroyTimelineSemaphore()");
        m_Timeline = timeline;
        m_TimelineCounter = initialSubmissionCount;
    }

    VKIT_NO_DISCARD Result<u64> UpdateCompletedTimeline();

    u64 GetCompletedTimeline() const
    {
        return m_CompletedTimeline;
    }
    u64 GetPendingTimeline() const
    {
        return m_TimelineCounter - m_CompletedTimeline;
    }

    VkSemaphore GetTimelineSempahore() const
    {
        return m_Timeline;
    }
    bool HasTimelineSemaphore() const
    {
        return m_Timeline != VK_NULL_HANDLE;
    }

  private:
    ProxyDevice m_Device{};
    VkQueue m_Queue = VK_NULL_HANDLE;
    VkSemaphore m_Timeline = VK_NULL_HANDLE;
    u64 m_TimelineCounter = 0;
    u64 m_TimelineSubmissions = 0;
    u64 m_CompletedTimeline = 0;
    u32 m_Family = 0;
};
} // namespace VKit
