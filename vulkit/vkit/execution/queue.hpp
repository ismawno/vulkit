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

const char *ToString(QueueType p_Type);

class LogicalDevice;

// not meant to be created by user, only by logical device.
// timeline semaphore must be explicitly submitted. it wont do it for you. the only convenience is to have a timeline
// handle and counter per queue
class Queue
{
  public:
    Queue() = default;

    Queue(const ProxyDevice &p_Device, const VkQueue p_Queue, const u32 p_Family)
        : m_Device(p_Device), m_Queue(p_Queue), m_Family(p_Family)
    {
    }

    VKIT_NO_DISCARD Result<> Submit(TKit::Span<const VkSubmitInfo> p_Info, VkFence p_Fence = VK_NULL_HANDLE);
#if defined(VKIT_API_VERSION_1_3) || defined(VK_KHR_synchronization2)
    VKIT_NO_DISCARD Result<> Submit2(TKit::Span<const VkSubmitInfo2KHR> p_Info, VkFence p_Fence = VK_NULL_HANDLE);
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

    u64 GetTotalSubmissionCount() const
    {
        return m_SubmissionCount;
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

    void TakeTimelineSemaphoreOwnership(const VkSemaphore p_Timeline, const u64 p_InitialSubmissionCount = 0)
    {
        m_Timeline = p_Timeline;
        m_SubmissionCount = p_InitialSubmissionCount;
    }

    VKIT_NO_DISCARD Result<u64> GetCompletedSubmissionCount() const;
    VKIT_NO_DISCARD Result<u64> GetPendingSubmissionCount() const;

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
    u64 m_SubmissionCount = 0;
    u32 m_Family = 0;
};
} // namespace VKit
