#include "vkit/core/pch.hpp"
#include "vkit/backend/logical_device.hpp"

namespace VKit
{
LogicalDevice::Builder::Builder(const Instance *p_Instance, const PhysicalDevice *p_PhysicalDevice) noexcept
    : m_Instance(p_Instance), m_PhysicalDevice(p_PhysicalDevice)
{
    for (usize i = 0; i < p_PhysicalDevice->GetInfo().QueueFamilies.size(); ++i)
    {
        const auto &family = p_PhysicalDevice->GetInfo().QueueFamilies[i];
        QueuePriorities priorities;
        priorities.Index = i;
        priorities.Priorities.resize(1, 1.0f);
        m_QueuePriorities.push_back(priorities);
    }
}

Result<LogicalDevice> LogicalDevice::Builder::Build() const noexcept
{
    const Instance::Info &instanceInfo = m_Instance->GetInfo();
    PhysicalDevice::Info devInfo = m_PhysicalDevice->GetInfo();

    DynamicArray<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(m_QueuePriorities.size());
    for (const QueuePriorities &priorities : m_QueuePriorities)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = priorities.Index;
        queueCreateInfo.queueCount = static_cast<u32>(priorities.Priorities.size());
        queueCreateInfo.pQueuePriorities = priorities.Priorities.data();
        queueCreateInfos.push_back(queueCreateInfo);
    }

    DynamicArray<const char *> enabledExtensions;
    enabledExtensions.reserve(devInfo.EnabledExtensions.size());
    for (const std::string &extension : devInfo.EnabledExtensions)
        enabledExtensions.push_back(extension.c_str());

    const bool v11 = instanceInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 1, 0);
    const bool prop2 = instanceInfo.Flags & InstanceFlags_Properties2Extension;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    VkPhysicalDeviceFeatures2 featuresChain{};
    if (v11 || prop2)
    {
        featuresChain.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        featuresChain.features = devInfo.EnabledFeatures.Core;
#ifdef VKIT_API_VERSION_1_2
        featuresChain.pNext = &devInfo.EnabledFeatures.Vulkan11;
        devInfo.EnabledFeatures.Vulkan11.pNext = &devInfo.EnabledFeatures.Vulkan12;
#endif
#ifdef VKIT_API_VERSION_1_3
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

    const auto createDevice = m_Instance->GetFunction<PFN_vkCreateDevice>("vkCreateDevice");
    if (!createDevice)
        return Result<LogicalDevice>::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                            "Failed to get the vkCreateDevice function");

    const auto destroyDevice = m_Instance->GetFunction<PFN_vkDestroyDevice>("vkDestroyDevice");
    if (!destroyDevice)
        return Result<LogicalDevice>::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                            "Failed to get the vkDestroyDevice function");

    VkDevice device;
    const VkResult result =
        createDevice(m_PhysicalDevice->GetDevice(), &createInfo, instanceInfo.AllocationCallbacks, &device);
    if (result != VK_SUCCESS)
        return Result<LogicalDevice>::Error(result, "Failed to create the logical device");

    const auto getQueue = System::GetDeviceFunction<PFN_vkGetDeviceQueue>("vkGetDeviceQueue", device);
    if (!getQueue)
    {
        destroyDevice(device, instanceInfo.AllocationCallbacks);
        return Result<LogicalDevice>::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                            "Failed to get the vkGetDeviceQueue function");
    }

    QueueArray queues;
    for (u32 i = 0; i < 4; ++i)
        for (u32 j = 0; j < VKIT_MAX_QUEUES_PER_FAMILY; ++j)
            getQueue(device, i, j, &queues[i * VKIT_MAX_QUEUES_PER_FAMILY + j]);

    return Result<LogicalDevice>::Ok(*m_Instance, *m_PhysicalDevice, device, queues);
}

LogicalDevice::Builder &LogicalDevice::Builder::SetQueuePriorities(
    std::span<const QueuePriorities> p_QueuePriorities) noexcept
{
    m_QueuePriorities.clear();
    m_QueuePriorities.insert(m_QueuePriorities.end(), p_QueuePriorities.begin(), p_QueuePriorities.end());
    return *this;
}

LogicalDevice::LogicalDevice(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice,
                             const VkDevice p_Device, const QueueArray &p_Queues) noexcept
    : m_Instance(p_Instance), m_PhysicalDevice(p_PhysicalDevice), m_Device(p_Device), m_Queues(p_Queues)
{
}

static void destroy(const LogicalDevice::Proxy &p_Device) noexcept
{
    const auto destroyDevice = System::GetDeviceFunction<PFN_vkDestroyDevice>("vkDestroyDevice", p_Device);
    TKIT_ASSERT(destroyDevice, "Failed to get the vkDestroyDevice function");

    destroyDevice(p_Device, p_Device.AllocationCallbacks);
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

void LogicalDevice::SubmitForDeletion(DeletionQueue &p_Queue) noexcept
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

VkQueue LogicalDevice::GetQueue(const QueueType p_Type, const u32 p_QueueIndex) const noexcept
{
    const u32 index = static_cast<u32>(p_Type) * VKIT_MAX_QUEUES_PER_FAMILY + p_QueueIndex;
    return m_Queues[index];
}

LogicalDevice::Proxy::operator VkDevice() const noexcept
{
    return Device;
}

LogicalDevice::Proxy LogicalDevice::CreateProxy() const noexcept
{
    return {m_Device, m_Instance.GetInfo().AllocationCallbacks};
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
LogicalDevice::operator bool() const noexcept
{
    return m_Device != VK_NULL_HANDLE;
}

} // namespace VKit