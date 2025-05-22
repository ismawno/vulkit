#include "vkit/core/pch.hpp"
#include "vkit/vulkan/logical_device.hpp"

namespace VKit
{
Result<LogicalDevice> LogicalDevice::Create(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice,
                                            const TKit::Span<const QueuePriorities> p_QueuePriorities) noexcept
{
    const Instance::Info &instanceInfo = p_Instance.GetInfo();
    PhysicalDevice::Info devInfo = p_PhysicalDevice.GetInfo();

    TKit::StaticArray8<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (const QueuePriorities &priorities : p_QueuePriorities)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = priorities.Index;
        queueCreateInfo.queueCount = priorities.Priorities.GetSize();
        queueCreateInfo.pQueuePriorities = priorities.Priorities.GetData();
        queueCreateInfos.Append(queueCreateInfo);
    }

    TKit::StaticArray256<const char *> enabledExtensions;
    for (const std::string &extension : devInfo.EnabledExtensions)
        enabledExtensions.Append(extension.c_str());

#ifdef VKIT_API_VERSION_1_1
    const bool v11 = devInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 1, 0);
#else
    const bool v11 = false;
#endif
    const bool prop2 = instanceInfo.Flags & Instance::Flag_Properties2Extension;
#ifndef VK_KHR_get_physical_device_properties2
    if (prop2)
        return Result<LogicalDevice>::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                            "The 'VK_KHR_get_physical_device_properties2' extension is not supported");
#endif

#if defined(VKIT_API_VERSION_1_1) || defined(VK_KHR_get_physical_device_properties2)
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

#    ifdef VKIT_API_VERSION_1_1
    VkPhysicalDeviceFeatures2 featuresChain{};
#    else
    VkPhysicalDeviceFeatures2KHR featuresChain{};
#    endif

    if (v11 || prop2)
    {
#    ifdef VKIT_API_VERSION_1_1
        featuresChain.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
#    else
        featuresChain.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
#    endif
        featuresChain.features = devInfo.EnabledFeatures.Core;

#    ifdef VKIT_API_VERSION_1_2
        if (devInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 2, 0))
        {
            featuresChain.pNext = &devInfo.EnabledFeatures.Vulkan11;
            devInfo.EnabledFeatures.Vulkan11.pNext = &devInfo.EnabledFeatures.Vulkan12;
        }
#    endif

#    ifdef VKIT_API_VERSION_1_3
        if (devInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 3, 0))
            devInfo.EnabledFeatures.Vulkan12.pNext = &devInfo.EnabledFeatures.Vulkan13;
#    endif

        createInfo.pNext = &featuresChain;
    }
    else
        createInfo.pEnabledFeatures = &devInfo.EnabledFeatures.Core;
#else
    createInfo.pEnabledFeatures = &devInfo.EnabledFeatures.Core;
#endif

    createInfo.queueCreateInfoCount = queueCreateInfos.GetSize();
    createInfo.pQueueCreateInfos = queueCreateInfos.GetData();
    createInfo.enabledExtensionCount = enabledExtensions.GetSize();
    createInfo.ppEnabledExtensionNames = enabledExtensions.GetData();
    createInfo.enabledLayerCount = instanceInfo.EnabledLayers.GetSize();
    createInfo.ppEnabledLayerNames = instanceInfo.EnabledLayers.GetData();

    const Vulkan::InstanceTable *itable = &instanceInfo.Table;

    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(itable, vkCreateDevice, Result<LogicalDevice>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(itable, vkGetPhysicalDeviceFormatProperties, Result<LogicalDevice>);

    VkDevice device;
    const VkResult result =
        itable->CreateDevice(p_PhysicalDevice, &createInfo, instanceInfo.AllocationCallbacks, &device);
    if (result != VK_SUCCESS)
        return Result<LogicalDevice>::Error(result, "Failed to create the logical device");

    const Vulkan::DeviceTable table = Vulkan::DeviceTable::Create(device);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN((&table), vkDestroyDevice, Result<LogicalDevice>);

    if (!table.vkGetDeviceQueue)
    {
        table.DestroyDevice(device, instanceInfo.AllocationCallbacks);
        return Result<LogicalDevice>::Error(VK_ERROR_INCOMPATIBLE_DRIVER, "Failed to load Vulkan function: "
                                                                          "vkGetDeviceQueue");
    }

    return Result<LogicalDevice>::Ok(p_Instance, p_PhysicalDevice, table, device);
}

Result<LogicalDevice> LogicalDevice::Create(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice) noexcept
{
    const u32 queueFamilyCount = p_PhysicalDevice.GetInfo().QueueFamilies.GetSize();
    TKit::StaticArray8<QueuePriorities> queuePriorities;
    for (u32 i = 0; i < queueFamilyCount; ++i)
    {
        QueuePriorities priorities;
        priorities.Index = i;
        priorities.Priorities.Resize(1, 1.0f);
        queuePriorities.Append(priorities);
    }
    return Create(p_Instance, p_PhysicalDevice, queuePriorities);
}

LogicalDevice::LogicalDevice(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice,
                             const Vulkan::DeviceTable &p_Table, const VkDevice p_Device) noexcept
    : m_Instance(p_Instance), m_PhysicalDevice(p_PhysicalDevice), m_Table(p_Table), m_Device(p_Device)
{
}

static void destroy(const LogicalDevice::Proxy &p_Device) noexcept
{
    p_Device.Table->DestroyDevice(p_Device, p_Device.AllocationCallbacks);
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
void LogicalDevice::WaitIdle(const Proxy &p_Device) noexcept
{
    TKIT_ASSERT_RETURNS(p_Device.Table->DeviceWaitIdle(p_Device), VK_SUCCESS,
                        "[VULKIT] Failed to wait for the logical device to be idle");
}
void LogicalDevice::WaitIdle() const noexcept
{
    WaitIdle(*this);
}

void LogicalDevice::SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
{
    const Proxy proxy = CreateProxy();
    p_Queue.Push([proxy]() {
        WaitIdle(proxy);
        destroy(proxy);
    });
}

VkDevice LogicalDevice::GetHandle() const noexcept
{
    return m_Device;
}

#ifdef VK_KHR_surface
Result<PhysicalDevice::SwapChainSupportDetails> LogicalDevice::QuerySwapChainSupport(
    const VkSurfaceKHR p_Surface) const noexcept
{
    return m_PhysicalDevice.QuerySwapChainSupport(m_Instance, p_Surface);
}
#endif

Result<VkFormat> LogicalDevice::FindSupportedFormat(TKit::Span<const VkFormat> p_Candidates,
                                                    const VkImageTiling p_Tiling,
                                                    const VkFormatFeatureFlags p_Features) const noexcept
{
    const Vulkan::InstanceTable *table = &m_Instance.GetInfo().Table;

    for (const VkFormat format : p_Candidates)
    {
        VkFormatProperties props;
        table->GetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

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
    Proxy proxy;
    proxy.Device = m_Device;
    proxy.AllocationCallbacks = m_Instance.GetInfo().AllocationCallbacks;
    proxy.Table = &m_Table;
    return proxy;
}

const Vulkan::DeviceTable &LogicalDevice::GetTable() const noexcept
{
    return m_Table;
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
    m_Table.GetDeviceQueue(m_Device, p_FamilyIndex, p_QueueIndex, &queue);
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
