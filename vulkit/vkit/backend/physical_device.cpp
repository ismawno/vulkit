#include "vkit/core/pch.hpp"
#include "vkit/backend/physical_device.hpp"

namespace VKit
{
template <typename C, typename T> static bool contains(const C &p_Container, const T &p_Value) noexcept
{
    return std::find(p_Container.begin(), p_Container.end(), p_Value) != p_Container.end();
}

template <typename T> std::pair<const VkBool32 *, usize> getFeatureIterable(const T &p_Features) noexcept
{
    usize size;
    const VkBool32 *ptr;
    if constexpr (std::is_same_v<T, VkPhysicalDeviceFeatures>)
    {
        size = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
        ptr = reinterpret_cast<const VkBool32 *>(&p_Features);
    }
    else
    {
        const usize offset = sizeof(VkStructureType) + sizeof(void *);
        size = (sizeof(T) - offset) / sizeof(VkBool32);
        const std::byte *rawPtr = reinterpret_cast<const std::byte *>(&p_Features) + offset;
        ptr = reinterpret_cast<const VkBool32 *>(rawPtr);
    }
    return {ptr, size};
}

template <typename T> static bool isAnyFeatureSet(const T &p_Features) noexcept
{
    const auto [ptr, size] = getFeatureIterable(p_Features);
    for (usize i = 0; i < size; ++i)
        if (ptr[i])
            return true;
    return false;
}

template <typename T> static bool compareFeatureStructs(const T &p_Supported, const T &p_Requested) noexcept
{
    const auto [ptr1, size1] = getFeatureIterable(p_Supported);
    const auto [ptr2, size2] = getFeatureIterable(p_Requested);
    TKIT_ASSERT(size1 == size2, "Feature struct sizes do not match");

    for (usize i = 0; i < size1; ++i)
        if (!ptr1[i] && ptr2[i])
            return false;
    return true;
}

static Result<PhysicalDevice::SwapChainSupportDetails> querySwapChainSupport(const Instance &p_Instance,
                                                                             const VkPhysicalDevice p_Device,
                                                                             const VkSurfaceKHR p_Surface) noexcept
{
    using Res = Result<PhysicalDevice::SwapChainSupportDetails>;
    const auto querySurfaceFormats =
        p_Instance.GetFunction<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>("vkGetPhysicalDeviceSurfaceFormatsKHR");
    const auto queryPresentModes = p_Instance.GetFunction<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
        "vkGetPhysicalDeviceSurfacePresentModesKHR");
    const auto queryCapabilities = p_Instance.GetFunction<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
        "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

    if (!querySurfaceFormats || !queryPresentModes || !queryCapabilities)
        return Res::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                          "Failed to get the required functions to query swap chain support");

    u32 formatCount = 0;
    u32 modeCount = 0;

    VkResult result = querySurfaceFormats(p_Device, p_Surface, &formatCount, nullptr);
    if (result != VK_SUCCESS)
        return Res::Error(result, "Failed to get the number of surface formats");

    result = queryPresentModes(p_Device, p_Surface, &modeCount, nullptr);
    if (result != VK_SUCCESS)
        return Res::Error(result, "Failed to get the number of present modes");

    if (formatCount == 0 || modeCount == 0)
        return Res::Error(VK_ERROR_INITIALIZATION_FAILED, "No surface formats or present modes found");

    PhysicalDevice::SwapChainSupportDetails details;
    result = queryCapabilities(p_Device, p_Surface, &details.Capabilities);
    if (result != VK_SUCCESS)
        return Res::Error(result, "Failed to get the surface capabilities");

    details.Formats.resize(formatCount);
    details.PresentModes.resize(modeCount);

    result = querySurfaceFormats(p_Device, p_Surface, &formatCount, details.Formats.data());
    if (result != VK_SUCCESS)
        return Res::Error(result, "Failed to get the surface formats");

    result = queryPresentModes(p_Device, p_Surface, &modeCount, details.PresentModes.data());
    if (result != VK_SUCCESS)
        return Res::Error(result, "Failed to get the present modes");

    return Res::Ok(details);
}

PhysicalDevice::Selector::Selector(const Instance *p_Instance) noexcept : m_Instance(p_Instance)
{
#ifdef VKIT_API_VERSION_1_2
    m_RequiredFeatures.Vulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    m_RequiredFeatures.Vulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
#endif
#ifdef VKIT_API_VERSION_1_3
    m_RequiredFeatures.Vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
#endif

    if (!(p_Instance->GetInfo().Flags & Instance::Flag_Headless))
        m_Flags |= Flag_RequirePresentQueue;
}

FormattedResult<PhysicalDevice> PhysicalDevice::Selector::Select() const noexcept
{
    const auto result = Enumerate();
    if (!result)
        return FormattedResult<PhysicalDevice>::Error(result.GetError().Result, result.GetError().Message);

    const auto &devices = result.GetValue();
    return devices[0];
}

FormattedResult<PhysicalDevice> PhysicalDevice::Selector::judgeDevice(const VkPhysicalDevice p_Device) const noexcept
{
    using JudgeResult = FormattedResult<PhysicalDevice>;
    const Instance::Info &instanceInfo = m_Instance->GetInfo();

    VkPhysicalDeviceProperties quickProperties;
    vkGetPhysicalDeviceProperties(p_Device, &quickProperties);
    const char *name = quickProperties.deviceName;

    if (m_Name != nullptr && strcmp(m_Name, name) != 0)
        return JudgeResult::Error(VK_ERROR_INITIALIZATION_FAILED, "The device name does not match the requested name");

    u32 extensionCount;
    VkResult result = vkEnumerateDeviceExtensionProperties(p_Device, nullptr, &extensionCount, nullptr);
    if (result != VK_SUCCESS)
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(result, "Failed to get the number of device extensions for the device: {}", name));

    TKit::StaticArray256<VkExtensionProperties> extensionsProps{extensionCount};
    result = vkEnumerateDeviceExtensionProperties(p_Device, nullptr, &extensionCount, extensionsProps.data());
    if (result != VK_SUCCESS)
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(result, "Failed to get the device extensions for the device: {}", name));

    TKit::StaticArray256<std::string> availableExtensions;
    for (const VkExtensionProperties &extension : extensionsProps)
        availableExtensions.push_back(extension.extensionName);

    TKit::StaticArray256<std::string> enabledExtensions;
    bool skipDevice = false;
    for (const std::string &extension : m_RequiredExtensions)
    {
        if (!contains(availableExtensions, extension))
        {
            skipDevice = true;
            break;
        }
        enabledExtensions.push_back(extension);
    }
    if (skipDevice)
        return JudgeResult::Error(VKIT_FORMAT_ERROR(
            VK_ERROR_EXTENSION_NOT_PRESENT, "The required extensions are not supported for the device: {}", name));

    bool fullySuitable = true;
    for (const std::string &extension : m_RequestedExtensions)
        if (contains(availableExtensions, extension))
            enabledExtensions.push_back(extension);
        else
            fullySuitable = false;

    const auto checkFlag = [this](const FlagBits p_Flag) -> bool { return m_Flags & p_Flag; };

    if (checkFlag(Flag_PortabilitySubset) && contains(availableExtensions, "VK_KHR_portability_subset"))
        enabledExtensions.push_back("VK_KHR_portability_subset");

    if (checkFlag(Flag_RequirePresentQueue))
        enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    u32 familyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(p_Device, &familyCount, nullptr);

    TKit::StaticArray8<VkQueueFamilyProperties> families{familyCount};
    vkGetPhysicalDeviceQueueFamilyProperties(p_Device, &familyCount, families.data());

    const auto compatibleQueueIndex = [&families, familyCount](const VkQueueFlags p_Flags) -> u32 {
        for (u32 i = 0; i < familyCount; ++i)
            if (families[i].queueCount > 0 && (families[i].queueFlags & p_Flags) == p_Flags)
                return i;

        return UINT32_MAX;
    };
    const auto dedicatedQueueIndex = [&families, familyCount](const VkQueueFlags p_Flags,
                                                              const VkQueueFlags p_ForbiddenFlags) -> u32 {
        for (u32 i = 0; i < familyCount; ++i)
            if (families[i].queueCount > 0 && (families[i].queueFlags & p_Flags) == p_Flags &&
                !(families[i].queueFlags & p_ForbiddenFlags))
                return i;

        return UINT32_MAX;
    };
    const auto separatedQueueIndex = [&families, familyCount](const VkQueueFlags p_Flags,
                                                              const VkQueueFlags p_ForbiddenFlags) -> u32 {
        u32 index = UINT32_MAX;
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
    const auto presentQueueIndex = [this, familyCount, p_Device](const VkSurfaceKHR p_Surface) -> u32 {
        if (!p_Surface)
            return UINT32_MAX;

        const auto queryPresentSupport =
            m_Instance->GetFunction<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>("vkGetPhysicalDeviceSurfaceSupportKHR");
        if (!queryPresentSupport)
            return UINT32_MAX;

        for (u32 i = 0; i < familyCount; ++i)
        {
            VkBool32 presentSupport = VK_FALSE;
            const VkResult result = queryPresentSupport(p_Device, i, p_Surface, &presentSupport);
            if (result == VK_SUCCESS && presentSupport == VK_TRUE)
                return i;
        }
        return UINT32_MAX;
    };

    TKIT_ASSERT(checkFlag(Flag_RequireComputeQueue) ||
                    (!checkFlag(Flag_RequireDedicatedComputeQueue) && !checkFlag(Flag_RequireSeparateComputeQueue)),
                "Flags mismatch: Must require compute queue to request dedicated or separate compute queue");

    TKIT_ASSERT(checkFlag(Flag_RequireTransferQueue) ||
                    (!checkFlag(Flag_RequireDedicatedTransferQueue) && !checkFlag(Flag_RequireSeparateTransferQueue)),
                "Flags mismatch: Must require transfer queue to request dedicated or separate transfer queue");

    u16 deviceFlags = 0;
    const u32 dedicatedCompute =
        dedicatedQueueIndex(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);
    const u32 dedicatedTransfer =
        dedicatedQueueIndex(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

    const u32 separateCompute = separatedQueueIndex(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT);
    const u32 separateTransfer = separatedQueueIndex(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT);

    const u32 computeCompatible = compatibleQueueIndex(VK_QUEUE_COMPUTE_BIT);
    const u32 transferCompatible = compatibleQueueIndex(VK_QUEUE_TRANSFER_BIT);

    const u32 graphicsIndex = compatibleQueueIndex(VK_QUEUE_GRAPHICS_BIT);
    const u32 presentIndex = presentQueueIndex(m_Surface);
    u32 computeIndex = UINT32_MAX;
    u32 transferIndex = UINT32_MAX;

    if (graphicsIndex != UINT32_MAX)
        deviceFlags |= PhysicalDevice::Flag_HasGraphicsQueue;
    if (presentIndex != UINT32_MAX)
        deviceFlags |= PhysicalDevice::Flag_HasPresentQueue;

    if (dedicatedCompute != UINT32_MAX)
    {
        computeIndex = dedicatedCompute;
        deviceFlags |= PhysicalDevice::Flag_HasDedicatedComputeQueue;
        deviceFlags |= PhysicalDevice::Flag_HasComputeQueue;
    }
    else if (separateCompute != UINT32_MAX)
    {
        computeIndex = separateCompute;
        deviceFlags |= PhysicalDevice::Flag_HasSeparateComputeQueue;
        deviceFlags |= PhysicalDevice::Flag_HasComputeQueue;
    }
    else if (computeCompatible != UINT32_MAX)
    {
        computeIndex = computeCompatible;
        deviceFlags |= PhysicalDevice::Flag_HasComputeQueue;
    }

    if (dedicatedTransfer != UINT32_MAX)
    {
        transferIndex = dedicatedTransfer;
        deviceFlags |= PhysicalDevice::Flag_HasDedicatedTransferQueue;
        deviceFlags |= PhysicalDevice::Flag_HasTransferQueue;
    }
    else if (separateTransfer != UINT32_MAX)
    {
        transferIndex = separateTransfer;
        deviceFlags |= PhysicalDevice::Flag_HasSeparateTransferQueue;
        deviceFlags |= PhysicalDevice::Flag_HasTransferQueue;
    }
    else if (transferCompatible != UINT32_MAX)
    {
        transferIndex = transferCompatible;
        deviceFlags |= PhysicalDevice::Flag_HasTransferQueue;
    }

    const auto checkFlags = [this, deviceFlags](const u16 p_SelectorFlag, const u16 p_DeviceFlag) -> bool {
        return !(m_Flags & p_SelectorFlag) || (deviceFlags & p_DeviceFlag);
    };

    if (!checkFlags(Flag_RequireGraphicsQueue, PhysicalDevice::Flag_HasGraphicsQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED, "The device {} does not have a graphics queue", name));
    if (!checkFlags(Flag_RequireComputeQueue, PhysicalDevice::Flag_HasComputeQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED, "The device {} does not have a compute queue", name));
    if (!checkFlags(Flag_RequireTransferQueue, PhysicalDevice::Flag_HasTransferQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED, "The device {} does not have a transfer queue", name));
    if (!checkFlags(Flag_RequirePresentQueue, PhysicalDevice::Flag_HasPresentQueue))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED, "The device {} does not have a present queue", name));

    if (!checkFlags(Flag_RequireDedicatedComputeQueue, PhysicalDevice::Flag_HasDedicatedComputeQueue))
        return JudgeResult::Error(VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED,
                                                    "The device {} does not have a dedicated compute queue", name));
    if (!checkFlags(Flag_RequireDedicatedTransferQueue, PhysicalDevice::Flag_HasDedicatedTransferQueue))
        return JudgeResult::Error(VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED,
                                                    "The device {} does not have a dedicated transfer queue", name));
    if (!checkFlags(Flag_RequireSeparateComputeQueue, PhysicalDevice::Flag_HasSeparateComputeQueue))
        return JudgeResult::Error(VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED,
                                                    "The device {} does not have a separate compute queue", name));
    if (!checkFlags(Flag_RequireSeparateTransferQueue, PhysicalDevice::Flag_HasSeparateTransferQueue))
        return JudgeResult::Error(VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED,
                                                    "The device {} does not have a separate transfer queue", name));

    if (checkFlag(Flag_RequirePresentQueue))
    {
        const auto qresult = querySwapChainSupport(*m_Instance, p_Device, m_Surface);
        if (!qresult)
            return JudgeResult::Error(
                VKIT_FORMAT_ERROR(qresult.GetError().Result, "{}. Device: {}", qresult.GetError().Message, name));
    }

    const bool v11 = instanceInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 1, 0);
    const bool prop2 = instanceInfo.Flags & Instance::Flag_Properties2Extension;

    Features features{};
    Properties properties{};

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

    if (v11 || prop2)
    {
        VkPhysicalDeviceFeatures2 featuresChain{};
        VkPhysicalDeviceProperties2 propertiesChain{};
        featuresChain.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        propertiesChain.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        // 2 and 2KHR have the same signature
        PFN_vkGetPhysicalDeviceFeatures2 getFeatures2;
        PFN_vkGetPhysicalDeviceProperties2 getProperties2;

        if (v11)
        {
            getFeatures2 = m_Instance->GetFunction<PFN_vkGetPhysicalDeviceFeatures2>("vkGetPhysicalDeviceFeatures2");
            getProperties2 =
                m_Instance->GetFunction<PFN_vkGetPhysicalDeviceProperties2>("vkGetPhysicalDeviceProperties2");
        }
        else
        {
            getFeatures2 =
                m_Instance->GetFunction<PFN_vkGetPhysicalDeviceFeatures2KHR>("vkGetPhysicalDeviceFeatures2KHR");
            getProperties2 =
                m_Instance->GetFunction<PFN_vkGetPhysicalDeviceProperties2KHR>("vkGetPhysicalDeviceProperties2KHR");
        }
        if (!getFeatures2 || !getProperties2)
            return JudgeResult::Error(VKIT_FORMAT_ERROR(
                VK_ERROR_EXTENSION_NOT_PRESENT,
                "Failed to get the vkGetPhysicalDeviceFeatures2(KHR) function for the device: {}", name));

#ifdef VKIT_API_VERSION_1_2
        if (instanceInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 2, 0))
        {
            featuresChain.pNext = &features.Vulkan11;
            propertiesChain.pNext = &properties.Vulkan11;

            features.Vulkan11.pNext = &features.Vulkan12;
            properties.Vulkan11.pNext = &properties.Vulkan12;
        }
#endif
#ifdef VKIT_API_VERSION_1_3
        if (instanceInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 3, 0))
        {
            features.Vulkan12.pNext = &features.Vulkan13;
            properties.Vulkan12.pNext = &properties.Vulkan13;
        }
#endif

        getFeatures2(p_Device, &featuresChain);
        getProperties2(p_Device, &propertiesChain);

        features.Core = featuresChain.features;
        properties.Core = propertiesChain.properties;
    }
    else
    {
        vkGetPhysicalDeviceFeatures(p_Device, &features.Core);
        vkGetPhysicalDeviceProperties(p_Device, &properties.Core);
    }

    if (!compareFeatureStructs(features.Core, m_RequiredFeatures.Core))
        return JudgeResult::Error(VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED,
                                                    "The device {} does not have the required features", name));

#ifdef VKIT_API_VERSION_1_2
    if (instanceInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 2, 0) &&
        (!compareFeatureStructs(features.Vulkan11, m_RequiredFeatures.Vulkan11) ||
         !compareFeatureStructs(features.Vulkan12, m_RequiredFeatures.Vulkan12)))
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED,
                              "The device {} does not have the required Vulkan 1.1 or 1.2 features", name));
#endif
#ifdef VKIT_API_VERSION_1_3
    if (instanceInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 3, 0) &&
        !compareFeatureStructs(features.Vulkan13, m_RequiredFeatures.Vulkan13))
        return JudgeResult::Error(VKIT_FORMAT_ERROR(
            VK_ERROR_INITIALIZATION_FAILED, "The device {} does not have the required Vulkan 1.3 features", name));
#endif

    if (properties.Core.apiVersion < instanceInfo.ApiVersion)
        return JudgeResult::Error(VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED,
                                                    "The device {} does not support the required API version", name));
    if (m_PreferredType != Type(properties.Core.deviceType))
    {
        if (!checkFlag(Flag_AnyType))
            return JudgeResult::Error(
                VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED, "The device {} is not of the preferred type", name));
        fullySuitable = false;
    }

    vkGetPhysicalDeviceMemoryProperties(p_Device, &properties.Memory);
    TKIT_ASSERT(m_RequestedMemory >= m_RequiredMemory,
                "Requested memory must be greater than or equal to required memory");

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
        return JudgeResult::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED, "The device {} does not have device local memory", name));
    if (!hasRequiredMemory)
        return JudgeResult::Error(VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED,
                                                    "The device {} does not have the required memory size", name));

    fullySuitable &= hasRequestedMemory;
    if (fullySuitable)
        deviceFlags |= PhysicalDevice::Flag_Optimal;

#ifdef VKIT_API_VERSION_1_2
    features.Vulkan11.pNext = nullptr;
    features.Vulkan12.pNext = nullptr;
#endif
#ifdef VKIT_API_VERSION_1_3
    features.Vulkan13.pNext = nullptr;
#endif

    PhysicalDevice::Info deviceInfo{};
    deviceInfo.AvailableExtensions = availableExtensions;
    deviceInfo.EnabledExtensions = enabledExtensions;
    deviceInfo.Properties = properties;
    deviceInfo.Flags = deviceFlags;
    deviceInfo.GraphicsIndex = graphicsIndex;
    deviceInfo.ComputeIndex = computeIndex;
    deviceInfo.TransferIndex = transferIndex;
    deviceInfo.PresentIndex = presentIndex;
    deviceInfo.QueueFamilies = families;
    deviceInfo.Type = Type(properties.Core.deviceType);
    deviceInfo.AvailableFeatures = features;
    deviceInfo.EnabledFeatures = m_RequiredFeatures;
    deviceInfo.Properties = properties;

    return JudgeResult::Ok(p_Device, deviceInfo);
}

Result<TKit::StaticArray4<FormattedResult<PhysicalDevice>>> PhysicalDevice::Selector::Enumerate() const noexcept
{
    using EnumerateResult = Result<TKit::StaticArray4<FormattedResult<PhysicalDevice>>>;

    if ((m_Flags & Flag_RequirePresentQueue) && !m_Surface)
        return EnumerateResult::Error(VK_ERROR_INITIALIZATION_FAILED,
                                      "The surface must be set if the instance is not headless");

    TKit::StaticArray4<VkPhysicalDevice> vkdevices;

    u32 deviceCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(m_Instance->GetInstance(), &deviceCount, nullptr);
    if (result != VK_SUCCESS)
        return EnumerateResult::Error(result, "Failed to get the number of physical devices");

    vkdevices.resize(deviceCount);
    result = vkEnumeratePhysicalDevices(m_Instance->GetInstance(), &deviceCount, vkdevices.data());
    if (result != VK_SUCCESS)
        return EnumerateResult::Error(result, "Failed to get the physical devices");

    if (vkdevices.empty())
        return EnumerateResult::Error(VK_ERROR_DEVICE_LOST, "No physical devices found");

    TKit::StaticArray4<FormattedResult<PhysicalDevice>> devices;
    for (const VkPhysicalDevice vkdevice : vkdevices)
    {
        const auto judgeResult = judgeDevice(vkdevice);
        devices.push_back(judgeResult);
    }

    std::stable_partition(devices.begin(), devices.end(), [](const FormattedResult<PhysicalDevice> &p_Device) {
        return p_Device && (p_Device.GetValue().GetInfo().Flags & PhysicalDevice::Flag_Optimal);
    });
    return EnumerateResult::Ok(devices);
}

PhysicalDevice::PhysicalDevice(VkPhysicalDevice p_Device, const Info &p_Info) noexcept
    : m_Device(p_Device), m_Info(p_Info)
{
}

bool PhysicalDevice::IsExtensionSupported(const char *p_Extension) const noexcept
{
    return contains(m_Info.AvailableExtensions, p_Extension);
}
bool PhysicalDevice::IsExtensionEnabled(const char *p_Extension) const noexcept
{
    return contains(m_Info.EnabledExtensions, p_Extension);
}
bool PhysicalDevice::EnableExtension(const char *p_Extension) noexcept
{
    if (IsExtensionEnabled(p_Extension))
        return true;
    if (!IsExtensionSupported(p_Extension))
        return false;
    m_Info.EnabledExtensions.push_back(p_Extension);
    return true;
}

PhysicalDevice::operator VkPhysicalDevice() const noexcept
{
    return m_Device;
}
PhysicalDevice::operator bool() const noexcept
{
    return m_Device != VK_NULL_HANDLE;
}

VkPhysicalDevice PhysicalDevice::GetDevice() const noexcept
{
    return m_Device;
}
const PhysicalDevice::Info &PhysicalDevice::GetInfo() const noexcept
{
    return m_Info;
}

Result<PhysicalDevice::SwapChainSupportDetails> PhysicalDevice::QuerySwapChainSupport(
    const Instance &p_Instance, const VkSurfaceKHR p_Surface) const noexcept
{
    return querySwapChainSupport(p_Instance, m_Device, p_Surface);
}

PhysicalDevice::Selector &PhysicalDevice::Selector::SetName(const char *p_Name) noexcept
{
    m_Name = p_Name;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::PreferType(const Type p_Type) noexcept
{
    m_PreferredType = p_Type;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireExtension(const char *p_Extension) noexcept
{
    m_RequiredExtensions.push_back(p_Extension);
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireExtensions(
    const std::span<const char *const> p_Extensions) noexcept
{
    m_RequiredExtensions.insert(m_RequiredExtensions.end(), p_Extensions.begin(), p_Extensions.end());
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestExtension(const char *p_Extension) noexcept
{
    m_RequestedExtensions.push_back(p_Extension);
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestExtensions(
    const std::span<const char *const> p_Extensions) noexcept
{
    m_RequestedExtensions.insert(m_RequestedExtensions.end(), p_Extensions.begin(), p_Extensions.end());
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireMemory(const VkDeviceSize p_Size) noexcept
{
    m_RequiredMemory = p_Size;
    if (m_RequestedMemory < m_RequiredMemory)
        m_RequestedMemory = m_RequiredMemory;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestMemory(const VkDeviceSize p_Size) noexcept
{
    m_RequestedMemory = p_Size;
    if (m_RequestedMemory < m_RequiredMemory)
        m_RequiredMemory = m_RequestedMemory;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireFeatures(const Features &p_Features) noexcept
{
    m_RequiredFeatures = p_Features;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::SetFlags(const Flags p_Flags) noexcept
{
    m_Flags = p_Flags;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::AddFlags(const Flags p_Flags) noexcept
{
    m_Flags |= p_Flags;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RemoveFlags(const Flags p_Flags) noexcept
{
    m_Flags &= ~p_Flags;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::SetSurface(const VkSurfaceKHR p_Surface) noexcept
{
    m_Surface = p_Surface;
    return *this;
}

} // namespace VKit