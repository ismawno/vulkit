#include "vkit/core/pch.hpp"
#include "vkit/backend/logical_device.hpp"

namespace VKit
{
Result<LogicalDevice> LogicalDevice::Create(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice,
                                            std::span<const QueuePriorities> p_QueuePriorities) noexcept
{
    const Instance::Info &instanceInfo = p_Instance.GetInfo();
    PhysicalDevice::Info devInfo = p_PhysicalDevice.GetInfo();

    TKit::StaticArray8<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (const QueuePriorities &priorities : p_QueuePriorities)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = priorities.Index;
        queueCreateInfo.queueCount = static_cast<u32>(priorities.Priorities.size());
        queueCreateInfo.pQueuePriorities = priorities.Priorities.data();
        queueCreateInfos.push_back(queueCreateInfo);
    }

    TKit::StaticArray128<const char *> enabledExtensions;
    for (const std::string &extension : devInfo.EnabledExtensions)
        enabledExtensions.push_back(extension.c_str());

    const bool v11 = instanceInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 1, 0);
    const bool prop2 = instanceInfo.Flags & Instance::Flag_Properties2Extension;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    VkPhysicalDeviceFeatures2 featuresChain{};
    if (v11 || prop2)
    {
        featuresChain.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        featuresChain.features = devInfo.EnabledFeatures.Core;
#ifdef VKIT_API_VERSION_1_2
        if (instanceInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 2, 0))
        {
            featuresChain.pNext = &devInfo.EnabledFeatures.Vulkan11;
            devInfo.EnabledFeatures.Vulkan11.pNext = &devInfo.EnabledFeatures.Vulkan12;
        }
#endif
#ifdef VKIT_API_VERSION_1_3
        if (instanceInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 3, 0))
            devInfo.EnabledFeatures.Vulkan12.pNext = &devInfo.EnabledFeatures.Vulkan13;
#endif
        createInfo.pNext = &featuresChain;
    }
    else
        createInfo.pEnabledFeatures = &devInfo.EnabledFeatures.Core;

    createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<u32>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();
    createInfo.enabledLayerCount = static_cast<u32>(instanceInfo.EnabledLayers.size());
    createInfo.ppEnabledLayerNames = instanceInfo.EnabledLayers.data();

    VkDevice device;
    const VkResult result = vkCreateDevice(p_PhysicalDevice, &createInfo, instanceInfo.AllocationCallbacks, &device);
    if (result != VK_SUCCESS)
        return Result<LogicalDevice>::Error(result, "Failed to create the logical device");

    return Result<LogicalDevice>::Ok(p_Instance, p_PhysicalDevice, device);
}

Result<LogicalDevice> LogicalDevice::Create(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice) noexcept
{
    const usize queueFamilyCount = p_PhysicalDevice.GetInfo().QueueFamilies.size();
    TKit::StaticArray8<QueuePriorities> queuePriorities;
    for (usize i = 0; i < queueFamilyCount; ++i)
    {
        QueuePriorities priorities;
        priorities.Index = i;
        priorities.Priorities.resize(1, 1.0f);
        queuePriorities.push_back(priorities);
    }
    return Create(p_Instance, p_PhysicalDevice, queuePriorities);
}

LogicalDevice::LogicalDevice(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice,
                             const VkDevice p_Device) noexcept
    : m_Instance(p_Instance), m_PhysicalDevice(p_PhysicalDevice), m_Device(p_Device)
{
}

static void destroy(const LogicalDevice::Proxy &p_Device) noexcept
{
    vkDestroyDevice(p_Device, p_Device.AllocationCallbacks);
}

void LogicalDevice::Destroy() noexcept
{
    destroy(CreateProxy());
    m_Device = VK_NULL_HANDLE;
}

const Instance &LogicalDevice::GetInstance() const noexcept
{
    return m_Instance;
}
const PhysicalDevice &LogicalDevice::GetPhysicalDevice() const noexcept
{
    return m_PhysicalDevice;
}
void LogicalDevice::WaitIdle(const VkDevice p_Device) noexcept
{
    TKIT_ASSERT_RETURNS(vkDeviceWaitIdle(p_Device), VK_SUCCESS, "Failed to wait for the logical device to be idle");
}
void LogicalDevice::WaitIdle() const noexcept
{
    WaitIdle(m_Device);
}

void LogicalDevice::SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
{
    const Proxy proxy = CreateProxy();
    p_Queue.Push([proxy]() {
        WaitIdle(proxy);
        destroy(proxy);
    });
}

VkDevice LogicalDevice::GetDevice() const noexcept
{
    return m_Device;
}

Result<PhysicalDevice::SwapChainSupportDetails> LogicalDevice::QuerySwapChainSupport(
    const VkSurfaceKHR p_Surface) const noexcept
{
    return m_PhysicalDevice.QuerySwapChainSupport(m_Instance, p_Surface);
}

Result<VkFormat> LogicalDevice::FindSupportedFormat(std::span<const VkFormat> p_Candidates,
                                                    const VkImageTiling p_Tiling,
                                                    const VkFormatFeatureFlags p_Features) const noexcept
{
    for (const VkFormat format : p_Candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

        if (p_Tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & p_Features) == p_Features)
            return Result<VkFormat>::Ok(format);
        if (p_Tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & p_Features) == p_Features)
            return Result<VkFormat>::Ok(format);
    }
    return Result<VkFormat>::Error(VK_ERROR_FORMAT_NOT_SUPPORTED, "No supported format found");
}

LogicalDevice::Proxy::operator VkDevice() const noexcept
{
    return Device;
}
LogicalDevice::Proxy::operator bool() const noexcept
{
    return Device != VK_NULL_HANDLE;
}

LogicalDevice::Proxy LogicalDevice::CreateProxy() const noexcept
{
    return {m_Device, m_Instance.GetInfo().AllocationCallbacks};
}

VkQueue LogicalDevice::GetQueue(const QueueType p_Type, const u32 p_QueueIndex) const noexcept
{
    const PhysicalDevice::Info &info = m_PhysicalDevice.GetInfo();
    switch (p_Type)
    {
    case QueueType::Graphics:
        return GetQueue(info.GraphicsIndex, p_QueueIndex);
    case QueueType::Compute:
        return GetQueue(info.ComputeIndex, p_QueueIndex);
    case QueueType::Transfer:
        return GetQueue(info.TransferIndex, p_QueueIndex);
    case QueueType::Present:
        return GetQueue(info.PresentIndex, p_QueueIndex);
    }

    return VK_NULL_HANDLE;
}

VkQueue LogicalDevice::GetQueue(const u32 p_FamilyIndex, const u32 p_QueueIndex) const noexcept
{
    VkQueue queue;
    vkGetDeviceQueue(m_Device, p_FamilyIndex, p_QueueIndex, &queue);
    return queue;
}

LogicalDevice::operator VkDevice() const noexcept
{
    return m_Device;
}
LogicalDevice::operator LogicalDevice::Proxy() const noexcept
{
    return CreateProxy();
}
LogicalDevice::operator bool() const noexcept
{
    return m_Device != VK_NULL_HANDLE;
}

} // namespace VKit