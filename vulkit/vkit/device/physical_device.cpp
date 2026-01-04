#include "vkit/core/pch.hpp"
#include "vkit/device/physical_device.hpp"
#include "vkit/execution/queue.hpp"

#define EXPAND_VERSION(p_Version)                                                                                      \
    VKIT_API_VERSION_MAJOR(p_Version), VKIT_API_VERSION_MINOR(p_Version), VKIT_API_VERSION_PATCH(p_Version)
namespace VKit
{
template <typename C, typename T> static bool contains(const C &p_Container, const T &p_Value)
{
    return std::find(p_Container.begin(), p_Container.end(), p_Value) != p_Container.end();
}

template <typename T, typename Bool = const VkBool32> std::pair<Bool *, u32> getFeatureIterable(T &p_Features)
{
    u32 size;
    Bool *ptr;
    if constexpr (std::is_same_v<T, VkPhysicalDeviceFeatures>)
    {
        size = sizeof(VkPhysicalDeviceFeatures) / sizeof(Bool);
        ptr = reinterpret_cast<Bool *>(&p_Features);
    }
    else
    {
        const u32 offset = sizeof(VkStructureType) + sizeof(void *);
        size = (sizeof(T) - offset) / sizeof(Bool);

        if constexpr (std::is_const_v<T>)
        {
            const std::byte *rawPtr = reinterpret_cast<const std::byte *>(&p_Features) + offset;
            ptr = reinterpret_cast<Bool *>(rawPtr);
        }
        else
        {
            std::byte *rawPtr = reinterpret_cast<std::byte *>(&p_Features) + offset;
            ptr = reinterpret_cast<Bool *>(rawPtr);
        }
    }
    return {ptr, size};
}

template <typename T> void orFeatures(T &p_Dest, const T &p_Src)
{
    auto [ptr1, size1] = getFeatureIterable<T, VkBool32>(p_Dest);
    const auto [ptr2, size2] = getFeatureIterable(p_Src);
    TKIT_ASSERT(size1 == size2, "[VULKIT] Feature struct sizes do not match");

    for (u32 i = 0; i < size1; ++i)
        ptr1[i] |= ptr2[i];
}

template <typename T> static bool compareFeatureStructs(const T &p_Supported, const T &p_Requested)
{
    const auto [ptr1, size1] = getFeatureIterable(p_Supported);
    const auto [ptr2, size2] = getFeatureIterable(p_Requested);
    TKIT_ASSERT(size1 == size2, "[VULKIT] Feature struct sizes do not match");

    for (u32 i = 0; i < size1; ++i)
        if (!ptr1[i] && ptr2[i])
            return false;
    return true;
}

static bool compareFeatures(const DeviceFeatures &p_Supported, const DeviceFeatures &p_Requested)
{
    if (!compareFeatureStructs(p_Supported.Core, p_Requested.Core))
        return false;
#ifdef VKIT_API_VERSION_1_2
    if (!compareFeatureStructs(p_Supported.Vulkan11, p_Requested.Vulkan11))
        return false;
    if (!compareFeatureStructs(p_Supported.Vulkan12, p_Requested.Vulkan12))
        return false;
#endif
#ifdef VKIT_API_VERSION_1_3
    if (!compareFeatureStructs(p_Supported.Vulkan13, p_Requested.Vulkan13))
        return false;
#endif
#ifdef VKIT_API_VERSION_1_4
    if (!compareFeatureStructs(p_Supported.Vulkan14, p_Requested.Vulkan14))
        return false;
#endif
    return true;
}

#ifdef VK_KHR_surface
static Result<PhysicalDevice::SwapChainSupportDetails> querySwapChainSupport(const Vulkan::InstanceTable *p_Table,
                                                                             const VkPhysicalDevice p_Device,
                                                                             const VkSurfaceKHR p_Surface)
{
    using Res = Result<PhysicalDevice::SwapChainSupportDetails>;
    u32 formatCount = 0;
    u32 modeCount = 0;

    VkResult result = p_Table->GetPhysicalDeviceSurfaceFormatsKHR(p_Device, p_Surface, &formatCount, nullptr);
    if (result != VK_SUCCESS)
        return Res::Error(result, "Failed to get the number of surface formats");

    result = p_Table->GetPhysicalDeviceSurfacePresentModesKHR(p_Device, p_Surface, &modeCount, nullptr);
    if (result != VK_SUCCESS)
        return Res::Error(result, "Failed to get the number of present modes");

    if (formatCount == 0 || modeCount == 0)
        return Res::Error(VK_ERROR_INITIALIZATION_FAILED, "No surface formats or present modes found");

    PhysicalDevice::SwapChainSupportDetails details;
    result = p_Table->GetPhysicalDeviceSurfaceCapabilitiesKHR(p_Device, p_Surface, &details.Capabilities);
    if (result != VK_SUCCESS)
        return Res::Error(result, "Failed to get the surface capabilities");

    details.Formats.Resize(formatCount);
    details.PresentModes.Resize(modeCount);

    result = p_Table->GetPhysicalDeviceSurfaceFormatsKHR(p_Device, p_Surface, &formatCount, details.Formats.GetData());
    if (result != VK_SUCCESS)
        return Res::Error(result, "Failed to get the surface formats");

    result = p_Table->GetPhysicalDeviceSurfacePresentModesKHR(p_Device, p_Surface, &modeCount,
                                                              details.PresentModes.GetData());
    if (result != VK_SUCCESS)
        return Res::Error(result, "Failed to get the present modes");

    return details;
}
#endif

#if defined(VKIT_API_VERSION_1_1) || defined(VK_KHR_get_physical_device_properties2)
template <typename T2, typename T1> void createChain(T2 &p_Chain, T1 &p_Properties, const u32 p_ApiVersion)
{
#    ifndef VKIT_API_VERSION_1_2
    chain.pNext = p_Properties.Next;
    return;
#    else
    if (p_ApiVersion < VKIT_API_VERSION_1_2)
    {
        p_Chain.pNext = p_Properties.Next;
        return;
    }
    p_Chain.pNext = &p_Properties.Vulkan11;
    p_Properties.Vulkan11.pNext = &p_Properties.Vulkan12;
    p_Properties.Vulkan12.pNext = p_Properties.Next;
#        ifdef VKIT_API_VERSION_1_3
    if (p_ApiVersion >= VKIT_API_VERSION_1_3)
    {
        p_Properties.Vulkan12.pNext = &p_Properties.Vulkan13;
        p_Properties.Vulkan13.pNext = p_Properties.Next;
    }
#            ifdef VKIT_API_VERSION_1_4
    if (p_ApiVersion >= VKIT_API_VERSION_1_4)
    {
        p_Properties.Vulkan13.pNext = &p_Properties.Vulkan14;
        p_Properties.Vulkan14.pNext = p_Properties.Next;
    }
#            endif
#        endif
#    endif
}
VkPhysicalDeviceFeatures2KHR DeviceFeatures::CreateChain(const u32 p_ApiVersion)
{
    VkPhysicalDeviceFeatures2KHR features{};
    features.features = Core;
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
    createChain<VkPhysicalDeviceFeatures2KHR>(features, *this, p_ApiVersion);
    return features;
}

VkPhysicalDeviceProperties2KHR DeviceProperties::CreateChain(const u32 p_ApiVersion)
{
    VkPhysicalDeviceProperties2KHR properties{};
    properties.properties = Core;
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
    createChain<VkPhysicalDeviceProperties2KHR>(properties, *this, p_ApiVersion);
    return properties;
}
#endif

Result<PhysicalDevice> PhysicalDevice::Selector::Select() const
{
    const auto result = Enumerate();
    TKIT_RETURN_ON_ERROR(result);

    const auto &devices = result.GetValue();
    return devices[0];
}

Result<PhysicalDevice> PhysicalDevice::Selector::judgeDevice(const VkPhysicalDevice p_Device) const
{
    using JudgeResult = Result<PhysicalDevice>;
    const Instance::Info &instanceInfo = m_Instance->GetInfo();
    const Vulkan::InstanceTable *table = &instanceInfo.Table;

    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceProperties, JudgeResult);

    VkPhysicalDeviceProperties quickProperties;
    table->GetPhysicalDeviceProperties(p_Device, &quickProperties);
    const char *name = quickProperties.deviceName;

    if (m_Name && strcmp(m_Name, name) != 0)
        return JudgeResult::Error(VKIT_FORMAT_ERROR(
            VK_ERROR_INCOMPATIBLE_DRIVER, "The device name '{}' does not match the requested name '{}'", name, m_Name));

    TKIT_LOG_WARNING_IF(quickProperties.apiVersion < m_RequestedApiVersion,
                        "[VULKIT] The device '{}' does not support the requested API version {}.{}.{}", name,
                        EXPAND_VERSION(m_RequestedApiVersion));

    if (quickProperties.apiVersion < m_RequiredApiVersion)
        return JudgeResult::Error(VKIT_FORMAT_ERROR(
            VK_ERROR_INCOMPATIBLE_DRIVER, "The device '{}' does not support the required API version {}.{}.{}", name,
            EXPAND_VERSION(m_RequiredApiVersion)));

    bool fullySuitable = quickProperties.apiVersion >= m_RequestedApiVersion;

    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkEnumerateDeviceExtensionProperties, JudgeResult);

    u32 extensionCount;
    VkResult result = table->EnumerateDeviceExtensionProperties(p_Device, nullptr, &extensionCount, nullptr);
    if (result != VK_SUCCESS)
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(result, "Failed to get the number of device extensions for the device: {}", name));

    TKit::Array256<VkExtensionProperties> extensionsProps{extensionCount};
    result = table->EnumerateDeviceExtensionProperties(p_Device, nullptr, &extensionCount, extensionsProps.GetData());
    if (result != VK_SUCCESS)
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(result, "Failed to get the device extensions for the device: {}", name));

    TKit::Array256<std::string> availableExtensions;
    for (const VkExtensionProperties &extension : extensionsProps)
        availableExtensions.Append(extension.extensionName);

    TKit::Array256<std::string> enabledExtensions;
    for (const std::string &extension : m_RequiredExtensions)
    {
        if (!contains(availableExtensions, extension))
            return JudgeResult::Error(VKIT_FORMAT_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT,
                                                        "The device '{}' does not support the required extension '{}'",
                                                        name, extension));
        enabledExtensions.Append(extension);
    }

    for (const std::string &extension : m_RequestedExtensions)
        if (contains(availableExtensions, extension))
            enabledExtensions.Append(extension);
        else
        {
            TKIT_LOG_WARNING("[VULKIT] The device '{}' does not support the requested extension '{}'", name, extension);
            fullySuitable = false;
        }

    DeviceSelectorFlags flags = m_Flags;
    const auto checkFlags = [&flags](const DeviceSelectorFlags p_Flags) -> bool { return flags & p_Flags; };

    if (checkFlags(DeviceSelectorFlag_PortabilitySubset) && contains(availableExtensions, "VK_KHR_portability_subset"))
        enabledExtensions.Append("VK_KHR_portability_subset");

    if (checkFlags(DeviceSelectorFlag_RequirePresentQueue))
        enabledExtensions.Append("VK_KHR_swapchain");

    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceQueueFamilyProperties, JudgeResult);

    u32 familyCount;
    table->GetPhysicalDeviceQueueFamilyProperties(p_Device, &familyCount, nullptr);

    TKit::Array8<VkQueueFamilyProperties> families{familyCount};
    table->GetPhysicalDeviceQueueFamilyProperties(p_Device, &familyCount, families.GetData());

    const auto compatibleQueueIndex = [&](const VkQueueFlags p_Flags) -> u32 {
        for (u32 i = 0; i < familyCount; ++i)
            if (families[i].queueCount > 0 && (families[i].queueFlags & p_Flags) == p_Flags)
                return i;

        return TKIT_U32_MAX;
    };
    const auto dedicatedQueueIndex = [&families, familyCount](const VkQueueFlags p_Flags,
                                                              const VkQueueFlags p_ForbiddenFlags) -> u32 {
        for (u32 i = 0; i < familyCount; ++i)
            if (families[i].queueCount > 0 && (families[i].queueFlags & p_Flags) == p_Flags &&
                !(families[i].queueFlags & p_ForbiddenFlags))
                return i;

        return TKIT_U32_MAX;
    };
    const auto separatedQueueIndex = [&families, familyCount](const VkQueueFlags p_Flags,
                                                              const VkQueueFlags p_ForbiddenFlags) -> u32 {
        u32 index = TKIT_U32_MAX;
        for (u32 i = 0; i < familyCount; ++i)
            if (families[i].queueCount > 0 && (families[i].queueFlags & p_Flags) == p_Flags &&
                !(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                if (!(families[i].queueFlags & p_ForbiddenFlags))
                    return i;
                index = i;
            }
        return index;
    };

#ifdef VK_KHR_surface
    const auto presentQueueIndex = [familyCount, p_Device, &table](const VkSurfaceKHR p_Surface) -> u32 {
        if (!p_Surface || !table->vkGetPhysicalDeviceSurfaceSupportKHR)
            return TKIT_U32_MAX;

        for (u32 i = 0; i < familyCount; ++i)
        {
            VkBool32 presentSupport = VK_FALSE;
            const VkResult result = table->GetPhysicalDeviceSurfaceSupportKHR(p_Device, i, p_Surface, &presentSupport);
            if (result == VK_SUCCESS && presentSupport == VK_TRUE)
                return i;
        }
        return TKIT_U32_MAX;
    };
#endif

    if (checkFlags(DeviceSelectorFlag_RequireDedicatedComputeQueue | DeviceSelectorFlag_RequireSeparateComputeQueue))
        flags |= DeviceSelectorFlag_RequireComputeQueue;
    if (checkFlags(DeviceSelectorFlag_RequireDedicatedTransferQueue | DeviceSelectorFlag_RequireSeparateTransferQueue))
        flags |= DeviceSelectorFlag_RequireTransferQueue;

    DeviceFlags deviceFlags = 0;
    const u32 dedicatedCompute =
        dedicatedQueueIndex(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);
    const u32 dedicatedTransfer =
        dedicatedQueueIndex(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

    const u32 separateCompute = separatedQueueIndex(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT);
    const u32 separateTransfer = separatedQueueIndex(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT);

    const u32 computeCompatible = compatibleQueueIndex(VK_QUEUE_COMPUTE_BIT);
    const u32 transferCompatible = compatibleQueueIndex(VK_QUEUE_TRANSFER_BIT);

    const u32 graphicsIndex = compatibleQueueIndex(VK_QUEUE_GRAPHICS_BIT);
#ifdef VK_KHR_surface
    const u32 presentIndex = presentQueueIndex(m_Surface);
#else
    const u32 presentIndex = TKIT_U32_MAX;
#endif
    u32 computeIndex = TKIT_U32_MAX;
    u32 transferIndex = TKIT_U32_MAX;

    if (graphicsIndex != TKIT_U32_MAX)
        deviceFlags |= DeviceFlag_HasGraphicsQueue;
    if (presentIndex != TKIT_U32_MAX)
        deviceFlags |= DeviceFlag_HasPresentQueue;

    if (dedicatedCompute != TKIT_U32_MAX)
    {
        computeIndex = dedicatedCompute;
        deviceFlags |= DeviceFlag_HasDedicatedComputeQueue;
        deviceFlags |= DeviceFlag_HasSeparateComputeQueue;
        deviceFlags |= DeviceFlag_HasComputeQueue;
    }
    else if (separateCompute != TKIT_U32_MAX)
    {
        computeIndex = separateCompute;
        deviceFlags |= DeviceFlag_HasSeparateComputeQueue;
        deviceFlags |= DeviceFlag_HasComputeQueue;
    }
    else if (computeCompatible != TKIT_U32_MAX)
    {
        computeIndex = computeCompatible;
        deviceFlags |= DeviceFlag_HasComputeQueue;
    }

    if (dedicatedTransfer != TKIT_U32_MAX)
    {
        transferIndex = dedicatedTransfer;
        deviceFlags |= DeviceFlag_HasDedicatedTransferQueue;
        deviceFlags |= DeviceFlag_HasSeparateTransferQueue;
        deviceFlags |= DeviceFlag_HasTransferQueue;
    }
    else if (separateTransfer != TKIT_U32_MAX)
    {
        transferIndex = separateTransfer;
        deviceFlags |= DeviceFlag_HasSeparateTransferQueue;
        deviceFlags |= DeviceFlag_HasTransferQueue;
    }
    else if (transferCompatible != TKIT_U32_MAX)
    {
        transferIndex = transferCompatible;
        deviceFlags |= DeviceFlag_HasTransferQueue;
    }

    const auto compareFlags = [&flags, deviceFlags](const DeviceSelectorFlags p_SelectorFlag,
                                                    const DeviceFlags p_DeviceFlag) -> bool {
        return !(flags & p_SelectorFlag) || (deviceFlags & p_DeviceFlag);
    };

    if (!compareFlags(DeviceSelectorFlag_RequireGraphicsQueue, DeviceFlag_HasGraphicsQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_DEVICE_LOST, "The device '{}' does not have a graphics queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequireComputeQueue, DeviceFlag_HasComputeQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_DEVICE_LOST, "The device '{}' does not have a compute queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequireTransferQueue, DeviceFlag_HasTransferQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_DEVICE_LOST, "The device '{}' does not have a transfer queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequirePresentQueue, DeviceFlag_HasPresentQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_DEVICE_LOST, "The device '{}' does not have a present queue", name));

    if (!compareFlags(DeviceSelectorFlag_RequireDedicatedComputeQueue, DeviceFlag_HasDedicatedComputeQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_DEVICE_LOST, "The device '{}' does not have a dedicated compute queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequireDedicatedTransferQueue, DeviceFlag_HasDedicatedTransferQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_DEVICE_LOST, "The device '{}' does not have a dedicated transfer queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequireSeparateComputeQueue, DeviceFlag_HasSeparateComputeQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_DEVICE_LOST, "The device '{}' does not have a separate compute queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequireSeparateTransferQueue, DeviceFlag_HasSeparateTransferQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_DEVICE_LOST, "The device '{}' does not have a separate transfer queue", name));

#ifdef VK_KHR_surface
    if (checkFlags(DeviceSelectorFlag_RequirePresentQueue))
    {
        VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceSurfaceFormatsKHR, JudgeResult);
        VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceSurfacePresentModesKHR, JudgeResult);
        VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceSurfaceCapabilitiesKHR, JudgeResult);
        const auto qresult = querySwapChainSupport(table, p_Device, m_Surface);
        if (!qresult)
            return JudgeResult::Error(VKIT_FORMAT_ERROR(qresult.GetError().ErrorCode, "{}. Device: {}",
                                                        qresult.GetError().GetMessage(), name));
    }
#endif

#ifdef VKIT_API_VERSION_1_1
    const bool v11 = quickProperties.apiVersion >= VKIT_API_VERSION_1_1;
#else
    const bool v11 = false;
#endif

    const bool prop2 = instanceInfo.Flags & InstanceFlag_Properties2Extension;
#ifndef VK_KHR_get_physical_device_properties2
    if (prop2)
        return JudgeResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                  "The 'VK_KHR_get_physical_device_properties2' extension is not supported");
#endif

    DeviceFeatures features{};
    DeviceProperties properties{};

#ifdef VKIT_API_VERSION_1_2
    features.Vulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    properties.Vulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
    features.Vulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    properties.Vulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
#endif
#ifdef VKIT_API_VERSION_1_3
    features.Vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    properties.Vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
#endif
#ifdef VKIT_API_VERSION_1_4
    features.Vulkan14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
    properties.Vulkan14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES;
#endif

#if defined(VKIT_API_VERSION_1_1) || defined(VK_KHR_get_physical_device_properties2)
    if (v11 || prop2)
    {
        VkPhysicalDeviceFeatures2KHR fchain = features.CreateChain(quickProperties.apiVersion);
        VkPhysicalDeviceProperties2KHR pchain = properties.CreateChain(quickProperties.apiVersion);

        if (v11)
        {
            VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceFeatures2, JudgeResult);
            VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceProperties2, JudgeResult);

            table->GetPhysicalDeviceFeatures2(p_Device, &fchain);
            table->GetPhysicalDeviceProperties2(p_Device, &pchain);
        }
        else
        {
            VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceFeatures2KHR, JudgeResult);
            VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceProperties2KHR, JudgeResult);

            table->GetPhysicalDeviceFeatures2KHR(p_Device, &fchain);
            table->GetPhysicalDeviceProperties2KHR(p_Device, &pchain);
        }

        features.Core = fchain.features;
        properties.Core = pchain.properties;
    }
    else
    {
        VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceFeatures, JudgeResult);
        VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceProperties, JudgeResult);

        table->GetPhysicalDeviceFeatures(p_Device, &features.Core);
        table->GetPhysicalDeviceProperties(p_Device, &properties.Core);
    }
#else
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceFeatures, JudgeResult);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceProperties, JudgeResult);

    table->GetPhysicalDeviceFeatures(p_Device, &features.Core);
    table->GetPhysicalDeviceProperties(p_Device, &properties.Core);
#endif

    if (!compareFeatureStructs(features.Core, m_RequiredFeatures.Core))
        return JudgeResult::Error(VKIT_FORMAT_ERROR(VK_ERROR_FEATURE_NOT_PRESENT,
                                                    "The device '{}' does not have the required core features", name));

#ifdef VKIT_API_VERSION_1_2
    if (!compareFeatureStructs(features.Vulkan11, m_RequiredFeatures.Vulkan11) ||
        !compareFeatureStructs(features.Vulkan12, m_RequiredFeatures.Vulkan12))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_FEATURE_NOT_PRESENT,
                              "The device '{}' does not have the required Vulkan 1.1 or 1.2 features", name));
#endif
#ifdef VKIT_API_VERSION_1_3
    if (!compareFeatureStructs(features.Vulkan13, m_RequiredFeatures.Vulkan13))
        return JudgeResult::Error(VKIT_FORMAT_ERROR(
            VK_ERROR_FEATURE_NOT_PRESENT, "The device '{}' does not have the required Vulkan 1.3 features", name));
#endif
#ifdef VKIT_API_VERSION_1_4
    if (!compareFeatureStructs(features.Vulkan14, m_RequiredFeatures.Vulkan14))
        return JudgeResult::Error(VKIT_FORMAT_ERROR(
            VK_ERROR_FEATURE_NOT_PRESENT, "The device '{}' does not have the required Vulkan 1.4 features", name));
#endif
    if (m_PreferredType != DeviceType(properties.Core.deviceType))
    {
        if (!checkFlags(DeviceSelectorFlag_AnyType))
            return JudgeResult::Error(
                VKIT_FORMAT_ERROR(VK_ERROR_DEVICE_LOST, "The device '{}' is not of the preferred type", name));
        fullySuitable = false;
    }

    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkGetPhysicalDeviceMemoryProperties, JudgeResult);
    table->GetPhysicalDeviceMemoryProperties(p_Device, &properties.Memory);

    TKIT_ASSERT(m_RequestedMemory >= m_RequiredMemory,
                "[VULKIT] Requested memory must be greater than or equal to required memory");

    bool hasDeviceLocalMemory = false;
    bool hasRequestedMemory = m_RequestedMemory == 0;
    bool hasRequiredMemory = m_RequiredMemory == 0;

    for (u32 i = 0; i < properties.Memory.memoryHeapCount; ++i)
    {
        if (!(properties.Memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT))
            continue;
        hasDeviceLocalMemory = true;
        if (hasRequestedMemory)
            break;

        const VkDeviceSize size = properties.Memory.memoryHeaps[i].size;
        if (size >= m_RequestedMemory)
            hasRequestedMemory = true;
        if (size >= m_RequiredMemory)
            hasRequiredMemory = true;
    }
    if (!hasDeviceLocalMemory)
        return JudgeResult::Error(VKIT_FORMAT_ERROR(VK_ERROR_OUT_OF_DEVICE_MEMORY,
                                                    "The device '{}' does not have device local memory", name));
    TKIT_LOG_WARNING_IF(!hasRequestedMemory, "[VULKIT] The device '{}' does not have the requested memory of {} bytes",
                        name, m_RequestedMemory);
    if (!hasRequiredMemory)
        return JudgeResult::Error(VKIT_FORMAT_ERROR(VK_ERROR_OUT_OF_DEVICE_MEMORY,
                                                    "The device '{}' does not have the required memory of {} bytes",
                                                    name, m_RequiredMemory));

    fullySuitable &= hasRequestedMemory;
    if (fullySuitable)
        deviceFlags |= DeviceFlag_Optimal;

#ifdef VKIT_API_VERSION_1_2
    features.Vulkan11.pNext = nullptr;
    features.Vulkan12.pNext = nullptr;
    properties.Vulkan11.pNext = nullptr;
    properties.Vulkan12.pNext = nullptr;
#endif
#ifdef VKIT_API_VERSION_1_3
    features.Vulkan13.pNext = nullptr;
    properties.Vulkan13.pNext = nullptr;
#endif
#ifdef VKIT_API_VERSION_1_4
    features.Vulkan14.pNext = nullptr;
    properties.Vulkan14.pNext = nullptr;
#endif

    PhysicalDevice::Info deviceInfo{};
    deviceInfo.ApiVersion = properties.Core.apiVersion;
    deviceInfo.AvailableExtensions = availableExtensions;
    deviceInfo.EnabledExtensions = enabledExtensions;
    deviceInfo.Flags = deviceFlags;
    deviceInfo.FamilyIndices[Queue_Graphics] = graphicsIndex;
    deviceInfo.FamilyIndices[Queue_Compute] = computeIndex;
    deviceInfo.FamilyIndices[Queue_Transfer] = transferIndex;
    deviceInfo.FamilyIndices[Queue_Present] = presentIndex;
    deviceInfo.QueueFamilies = families;
    deviceInfo.Type = DeviceType(properties.Core.deviceType);
    deviceInfo.AvailableFeatures = features;
    deviceInfo.EnabledFeatures = m_RequiredFeatures;
    deviceInfo.Properties = properties;

    return JudgeResult::Ok(p_Device, deviceInfo);
}

PhysicalDevice::Selector::Selector(const Instance *p_Instance) : m_Instance(p_Instance)
{
#ifdef VKIT_API_VERSION_1_2
    m_RequiredFeatures.Vulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    m_RequiredFeatures.Vulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
#endif
#ifdef VKIT_API_VERSION_1_3
    m_RequiredFeatures.Vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
#endif
#ifdef VKIT_API_VERSION_1_4
    m_RequiredFeatures.Vulkan14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
#endif

    if (!(m_Instance->GetInfo().Flags & InstanceFlag_Headless))
        m_Flags |= DeviceSelectorFlag_RequirePresentQueue;
}

Result<TKit::Array4<Result<PhysicalDevice>>> PhysicalDevice::Selector::Enumerate() const
{
    using EnumerateResult = Result<TKit::Array4<Result<PhysicalDevice>>>;

#ifdef VK_KHR_surface
    if ((m_Flags & DeviceSelectorFlag_RequirePresentQueue) && !m_Surface)
        return EnumerateResult::Error(
            VK_ERROR_INITIALIZATION_FAILED,
            "The surface must be set if the instance is not headless (requires present queue)");
#else
    if (m_Flags & DeviceSelectorFlag_RequirePresentQueue)
        return EnumerateResult::Error(VK_ERROR_INITIALIZATION_FAILED,
                                      "A present queue is not available with a device that does not support the "
                                      "surface extension. The instance must be headless");
#endif

    TKit::Array4<VkPhysicalDevice> vkdevices;

    const Vulkan::InstanceTable *table = &m_Instance->GetInfo().Table;
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(table, vkEnumeratePhysicalDevices, EnumerateResult);

    u32 deviceCount = 0;
    VkResult result = table->EnumeratePhysicalDevices(m_Instance->GetHandle(), &deviceCount, nullptr);
    if (result != VK_SUCCESS)
        return EnumerateResult::Error(result, "Failed to get the number of physical devices");

    vkdevices.Resize(deviceCount);
    result = table->EnumeratePhysicalDevices(m_Instance->GetHandle(), &deviceCount, vkdevices.GetData());
    if (result != VK_SUCCESS)
        return EnumerateResult::Error(result, "Failed to get the physical devices");

    if (vkdevices.IsEmpty())
        return EnumerateResult::Error(VK_ERROR_DEVICE_LOST, "No physical devices found");

    TKit::Array4<Result<PhysicalDevice>> devices;
    for (const VkPhysicalDevice vkdevice : vkdevices)
    {
        const auto judgeResult = judgeDevice(vkdevice);
        devices.Append(judgeResult);
    }

    std::stable_partition(devices.begin(), devices.end(), [](const Result<PhysicalDevice> &p_Device) {
        return p_Device && (p_Device.GetValue().GetInfo().Flags & DeviceFlag_Optimal);
    });
    return devices;
}

bool PhysicalDevice::AreFeaturesSupported(const DeviceFeatures &p_Features) const
{
    return compareFeatures(m_Info.AvailableFeatures, p_Features);
}
bool PhysicalDevice::AreFeaturesEnabled(const DeviceFeatures &p_Features) const
{
    return compareFeatures(m_Info.EnabledFeatures, p_Features);
}
bool PhysicalDevice::EnableFeatures(const DeviceFeatures &p_Features)
{
    if (!AreFeaturesSupported(p_Features))
        return false;

    orFeatures(m_Info.EnabledFeatures, p_Features);
    return true;
}

bool PhysicalDevice::IsExtensionSupported(const char *p_Extension) const
{
    return contains(m_Info.AvailableExtensions, p_Extension);
}
bool PhysicalDevice::IsExtensionEnabled(const char *p_Extension) const
{
    return contains(m_Info.EnabledExtensions, p_Extension);
}
bool PhysicalDevice::EnableExtension(const char *p_Extension)
{
    if (IsExtensionEnabled(p_Extension))
        return true;
    if (!IsExtensionSupported(p_Extension))
        return false;
    m_Info.EnabledExtensions.Append(p_Extension);
    return true;
}

#ifdef VK_KHR_surface
Result<PhysicalDevice::SwapChainSupportDetails> PhysicalDevice::QuerySwapChainSupport(
    const Instance::Proxy &p_Instance, const VkSurfaceKHR p_Surface) const
{
    return querySwapChainSupport(p_Instance.Table, m_Device, p_Surface);
}
#endif

PhysicalDevice::Selector &PhysicalDevice::Selector::SetName(const char *p_Name)
{
    m_Name = p_Name;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::PreferType(const DeviceType p_Type)
{
    m_PreferredType = p_Type;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireApiVersion(u32 p_Version)
{
    m_RequiredApiVersion = p_Version;
    if (m_RequestedApiVersion < m_RequiredApiVersion)
        m_RequestedApiVersion = m_RequiredApiVersion;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch)
{
    return RequireApiVersion(VKIT_MAKE_VERSION(0, p_Major, p_Minor, p_Patch));
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestApiVersion(u32 p_Version)
{
    m_RequestedApiVersion = p_Version;
    if (m_RequestedApiVersion < m_RequiredApiVersion)
        m_RequiredApiVersion = m_RequestedApiVersion;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch)
{
    return RequestApiVersion(VKIT_MAKE_VERSION(0, p_Major, p_Minor, p_Patch));
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireExtension(const char *p_Extension)
{
    m_RequiredExtensions.Append(p_Extension);
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireExtensions(const TKit::Span<const char *const> p_Extensions)
{
    m_RequiredExtensions.Insert(m_RequiredExtensions.end(), p_Extensions.begin(), p_Extensions.end());
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestExtension(const char *p_Extension)
{
    m_RequestedExtensions.Append(p_Extension);
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestExtensions(const TKit::Span<const char *const> p_Extensions)
{
    m_RequestedExtensions.Insert(m_RequestedExtensions.end(), p_Extensions.begin(), p_Extensions.end());
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireMemory(const VkDeviceSize p_Size)
{
    m_RequiredMemory = p_Size;
    if (m_RequestedMemory < m_RequiredMemory)
        m_RequestedMemory = m_RequiredMemory;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestMemory(const VkDeviceSize p_Size)
{
    m_RequestedMemory = p_Size;
    if (m_RequestedMemory < m_RequiredMemory)
        m_RequiredMemory = m_RequestedMemory;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireFeatures(const DeviceFeatures &p_Features)
{
    void *next = m_RequiredFeatures.Next;
    m_RequiredFeatures = p_Features;
    m_RequiredFeatures.Next = next;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::SetFlags(const DeviceSelectorFlags p_Flags)
{
    m_Flags = p_Flags;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::AddFlags(const DeviceSelectorFlags p_Flags)
{
    m_Flags |= p_Flags;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RemoveFlags(const DeviceSelectorFlags p_Flags)
{
    m_Flags &= ~p_Flags;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::SetSurface(const VkSurfaceKHR p_Surface)
{
    m_Surface = p_Surface;
    return *this;
}

} // namespace VKit
