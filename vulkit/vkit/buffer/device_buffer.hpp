#pragma once

#include "vkit/buffer/buffer.hpp"

#include <span>

namespace VKit
{
template <typename T> class DeviceBuffer
{
  public:
    struct Specs
    {
        std::span<const T> Data;
        VkBufferUsageFlags Usage;
        CommandPool *CommandPool;
        VkQueue Queue;
    };

    static RawResult<DeviceBuffer> Create(const Specs &p_Specs) noexcept
    {
        Buffer::Specs specs{};
        specs.InstanceCount = p_Specs.Data.size();
        specs.InstanceSize = sizeof(T);
        specs.Usage = p_Specs.Usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        specs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        const auto result1 = Buffer::Create(specs);
        if (!result1)
            return RawResult<DeviceBuffer>::Error(result1.GetError().Result, "Failed to create main device buffer");

        const Buffer &buffer = result1.GetValue();

        Buffer::Specs stagingSpecs = specs;
        stagingSpecs.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingSpecs.AllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingSpecs.AllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        auto result2 = Buffer::Create(stagingSpecs);
        if (!result2)
            return RawResult<DeviceBuffer>::Error(result2.GetError().Result, "Failed to create staging buffer");

        Buffer &stagingBuffer = result2.GetValue();
        stagingBuffer.Map();
        stagingBuffer.Write(p_Specs.Data.data());
        stagingBuffer.Flush();
        stagingBuffer.Unmap();

        const auto result3 = buffer.CopyFrom(stagingBuffer, *p_Specs.CommandPool, p_Specs.Queue);
        if (!result3)
            return RawResult<DeviceBuffer>::Error(result3.GetError().Result, "Failed to copy data to main buffer");

        return RawResult<DeviceBuffer>::Ok(buffer);
    }

    ~DeviceBuffer() noexcept
    {
        m_Buffer.Destroy();
    }

    VkBuffer GetBuffer() const noexcept
    {
        return m_Buffer.GetBuffer();
    }
    const Buffer::Info &GetInfo() const noexcept
    {
        return m_Buffer.GetInfo();
    }

  private:
    DeviceBuffer(const Buffer &p_Buffer) noexcept : m_Buffer(p_Buffer)
    {
    }
    Buffer m_Buffer;
};
} // namespace VKit