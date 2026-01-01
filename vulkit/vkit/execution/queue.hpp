#pragma once

#include "vkit/core/alias.hpp"
#include "vkit/device/proxy_device.hpp"

namespace VKit
{
enum QueueType : u32
{
    Queue_Graphics = 0,
    Queue_Compute = 1,
    Queue_Transfer = 2,
    Queue_Present = 3,
};

const char *ToString(QueueType p_Type);

class LogicalDevice;

// not meant to be created by user, only by logical device.
class Queue
{
  public:
    Queue() = default;

#ifdef VK_KHR_timeline_semaphore
    static Result<Queue> Create(const LogicalDevice &p_Device, VkQueue p_Queue, u32 p_Family, u32 p_Index);

    Queue(const ProxyDevice &p_Device, const VkQueue p_Queue, const u32 p_Family, const u32 p_Index,
          const VkSemaphore p_Timeline = VK_NULL_HANDLE)
        : m_Queue(p_Queue), m_Device(p_Device), m_Timeline(p_Timeline), m_Family(p_Family), m_Index(p_Index)
    {
    }
#else
    static Result<Queue> Create(VkQueue p_Queue, QueueType p_Type, u32 p_Family, u32 p_Index);
    Queue(const VkQueue p_Queue, const u32 p_Family, const u32 p_Index)
        : m_Queue(p_Queue), m_Family(p_Family), m_Index(p_Index)
    {
    }
#endif

    VkQueue GetHandle() const
    {
        return m_Queue;
    }

    u64 GetTotalSubmissionCount() const
    {
        return m_SubmissionCount;
    }

    u32 GetIndex() const
    {
        return m_Index;
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

#ifdef VK_KHR_timeline_semaphore
    Result<u64> GetCompletedSubmissionCount() const;
    Result<u64> GetPendingSubmissionCount() const;

    VkSemaphore GetTimelineSempahore() const
    {
        return m_Timeline;
    }
    bool HasTimelineSemaphore() const
    {
        return m_Timeline != VK_NULL_HANDLE;
    }
#else
    Result<u64> GetCompletedSubmissionCount() const
    {
        return u64(0);
    }
    Result<u64> GetPendingSubmissionCount() const
    {
        return u64(0);
    }
    VkSemaphore GetTimelineSempahore() const
    {
        return VK_NULL_HANDLE;
    }
    bool HasTimelineSemaphore() const
    {
        return false;
    }
#endif

  private:
    VkQueue m_Queue = VK_NULL_HANDLE;
#ifdef VK_KHR_timeline_semaphore
    ProxyDevice m_Device{};
    VkSemaphore m_Timeline = VK_NULL_HANDLE;
#endif
    u64 m_SubmissionCount = 0;
    u32 m_Family = 0;
    u32 m_Index = 0;
};
} // namespace VKit
