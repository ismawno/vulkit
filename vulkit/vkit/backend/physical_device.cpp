#include "vkit/core/pch.hpp"
#include "vkit/backend/physical_device.hpp"

namespace VKit
{
template <typename C, typename T> static bool contains(const C &p_Container, const T &p_Value) noexcept
{
    return std::find(p_Container.begin(), p_Container.end(), p_Value) != p_Container.end();
}

template <typename T> std::pair<VkBool32 *, usize> getFeatureIterable(const T &p_Features) noexcept
{
    usize size;
    VkBool32 *ptr;
    if constexpr (std::is_same_v<T, VkPhysicalDeviceFeatures>)
    {
        size = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
        ptr = reinterpret_cast<VkBool32 *>(&p_Features);
    }
    else
    {
        const usize offset = sizeof(VkStructureType) + sizeof(void *);
        size = (sizeof(T) - offset) / sizeof(VkBool32);
        std::byte *rawPtr = reinterpret_cast<std::byte *>(&p_Features) + offset;
        ptr = reinterpret_cast<VkBool32 *>(&p_Features);
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

PhysicalDevice::Selector::Selector(const Instance &p_Instance) noexcept : m_Instance(p_Instance)
{
#ifdef VKIT_API_VERSION_1_2
    m_RequiredFeatures.Vulkan11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    m_RequiredFeatures.Vulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
#endif
#ifdef VKIT_API_VERSION_1_3
    m_RequiredFeatures.Vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
#endif

    if ((p_Instance.GetInfo().Flags & InstanceFlags_Headless))
        m_Flags |= PhysicalDeviceSelectorFlags_RequirePresentQueue;
}

Result<PhysicalDevice> PhysicalDevice::Selector::Select() const noexcept
{
    const auto result = Enumerate();
    if (!result)
        return Result<PhysicalDevice>::Error(result.GetError());

    const DynamicArray<PhysicalDevice> &devices = result.GetValue();
    if (devices.empty())
        return Result<PhysicalDevice>::Error(VK_ERROR_DEVICE_LOST, "No physical devices found");

    return Result<PhysicalDevice>::Ok(devices[0]);
}

Result<DynamicArray<PhysicalDevice>> PhysicalDevice::Selector::Enumerate() const noexcept
{
    using EnumerateResult = Result<DynamicArray<PhysicalDevice>>;
    const Instance::Info &instanceInfo = m_Instance.GetInfo();

    const auto checkFlag = [this](const u16 p_Flag) -> bool { return m_Flags & p_Flag; };

    if (!checkFlag(PhysicalDeviceSelectorFlags_RequirePresentQueue) && !m_Surface)
        return EnumerateResult::Error(VK_ERROR_INITIALIZATION_FAILED,
                                      "The surface must be set if the instance is not headless");

    DynamicArray<VkPhysicalDevice> vkdevices;
    const auto enumerateDevices = m_Instance.GetFunction<PFN_vkEnumeratePhysicalDevices>("vkEnumeratePhysicalDevices");
    if (!enumerateDevices)
        return EnumerateResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                      "Failed to get the vkEnumeratePhysicalDevices function");

    u32 deviceCount = 0;
    VkResult result = enumerateDevices(m_Instance, &deviceCount, nullptr);
    if (result != VK_SUCCESS)
        return EnumerateResult::Error(result, "Failed to get the number of physical devices");

    vkdevices.resize(deviceCount);
    result = enumerateDevices(m_Instance, &deviceCount, vkdevices.data());
    if (result != VK_SUCCESS)
        return EnumerateResult::Error(result, "Failed to get the physical devices");

    if (vkdevices.empty())
        return EnumerateResult::Error(VK_ERROR_DEVICE_LOST, "No physical devices found");

    const auto enumerateExtensions =
        m_Instance.GetFunction<PFN_vkEnumerateDeviceExtensionProperties>("vkEnumerateDeviceExtensionProperties");
    if (!enumerateExtensions)
        return EnumerateResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                      "Failed to get the vkEnumerateDeviceExtensionProperties function");

    const auto queryFamilies = m_Instance.GetFunction<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
        "vkGetPhysicalDeviceQueueFamilyProperties");
    if (!queryFamilies)
        return EnumerateResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                      "Failed to get the vkGetPhysicalDeviceQueueFamilyProperties function");

    const auto getProperties =
        m_Instance.GetFunction<PFN_vkGetPhysicalDeviceProperties>("vkGetPhysicalDeviceProperties");
    if (!getProperties)
        return EnumerateResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                      "Failed to get the vkGetPhysicalDeviceProperties function");

    const auto getFeatures = m_Instance.GetFunction<PFN_vkGetPhysicalDeviceFeatures>("vkGetPhysicalDeviceFeatures");
    if (!getFeatures)
        return EnumerateResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                      "Failed to get the vkGetPhysicalDeviceFeatures function");

    const auto getMemoryProperties =
        m_Instance.GetFunction<PFN_vkGetPhysicalDeviceMemoryProperties>("vkGetPhysicalDeviceMemoryProperties");
    if (!getMemoryProperties)
        return EnumerateResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                      "Failed to get the vkGetPhysicalDeviceMemoryProperties function");

    DynamicArray<PhysicalDevice> devices;
    for (const VkPhysicalDevice vkdevice : vkdevices)
    {
        u32 extensionCount;
        result = enumerateExtensions(vkdevice, nullptr, &extensionCount, nullptr);
        if (result != VK_SUCCESS)
            return EnumerateResult::Error(result, "Failed to get the number of device extensions");

        DynamicArray<VkExtensionProperties> extensionsProps{extensionCount};
        result = enumerateExtensions(vkdevice, nullptr, &extensionCount, extensionsProps.data());
        if (result != VK_SUCCESS)
            return EnumerateResult::Error(result, "Failed to get the device extensions");

        DynamicArray<std::string> availableExtensions;
        availableExtensions.reserve(extensionCount);
        for (const VkExtensionProperties &extension : extensionsProps)
            availableExtensions.push_back(extension.extensionName);

        DynamicArray<std::string> enabledExtensions;
        enabledExtensions.reserve(availableExtensions.size());

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
            continue;

        bool fullySuitable = true;
        for (const std::string &extension : m_RequestedExtensions)
            if (contains(availableExtensions, extension))
                enabledExtensions.push_back(extension);
            else
                fullySuitable = false;

        if (checkFlag(PhysicalDeviceSelectorFlags_PortabilitySubset))
            enabledExtensions.push_back("VK_KHR_portability_subset");
        if (checkFlag(PhysicalDeviceSelectorFlags_RequirePresentQueue))
            enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        u32 familyCount;
        queryFamilies(vkdevice, &familyCount, nullptr);

        DynamicArray<VkQueueFamilyProperties> families{familyCount};
        queryFamilies(vkdevice, &familyCount, families.data());

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
        const auto presentQueueIndex = [this, &families, familyCount, vkdevice](const VkSurfaceKHR p_Surface) -> u32 {
            if (!p_Surface)
                return UINT32_MAX;

            const auto queryPresentSupport = m_Instance.GetFunction<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
                "vkGetPhysicalDeviceSurfaceSupportKHR");
            if (!queryPresentSupport)
                return UINT32_MAX;

            for (u32 i = 0; i < familyCount; ++i)
            {
                VkBool32 presentSupport = VK_FALSE;
                const VkResult result = queryPresentSupport(vkdevice, i, p_Surface, &presentSupport);
                if (result == VK_SUCCESS && presentSupport == VK_TRUE)
                    return i;
            }
            return UINT32_MAX;
        };

        TKIT_ASSERT(checkFlag(PhysicalDeviceSelectorFlags_RequireComputeQueue) ||
                        (!checkFlag(PhysicalDeviceSelectorFlags_DedicatedComputeQueue) &&
                         !checkFlag(PhysicalDeviceSelectorFlags_SeparateComputeQueue)),
                    "Flags mismatch: Must require compute queue to request dedicated or separate compute queue");

        TKIT_ASSERT(checkFlag(PhysicalDeviceSelectorFlags_RequireTransferQueue) ||
                        (!checkFlag(PhysicalDeviceSelectorFlags_DedicatedTransferQueue) &&
                         !checkFlag(PhysicalDeviceSelectorFlags_SeparateTransferQueue)),
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
            deviceFlags |= PhysicalDeviceFlags_HasGraphicsQueue;
        if (presentIndex != UINT32_MAX)
            deviceFlags |= PhysicalDeviceFlags_HasPresentQueue;

        if (dedicatedCompute != UINT32_MAX)
        {
            computeIndex = dedicatedCompute;
            deviceFlags |= PhysicalDeviceFlags_DedicatedComputeQueue;
            deviceFlags |= PhysicalDeviceFlags_HasComputeQueue;
        }
        else if (separateCompute != UINT32_MAX)
        {
            computeIndex = separateCompute;
            deviceFlags |= PhysicalDeviceFlags_SeparateComputeQueue;
            deviceFlags |= PhysicalDeviceFlags_HasComputeQueue;
        }
        else if (computeCompatible != UINT32_MAX)
        {
            computeIndex = computeCompatible;
            deviceFlags |= PhysicalDeviceFlags_HasComputeQueue;
        }

        if (dedicatedTransfer != UINT32_MAX)
        {
            transferIndex = dedicatedTransfer;
            deviceFlags |= PhysicalDeviceFlags_DedicatedTransferQueue;
            deviceFlags |= PhysicalDeviceFlags_HasTransferQueue;
        }
        else if (separateTransfer != UINT32_MAX)
        {
            transferIndex = separateTransfer;
            deviceFlags |= PhysicalDeviceFlags_SeparateTransferQueue;
            deviceFlags |= PhysicalDeviceFlags_HasTransferQueue;
        }
        else if (transferCompatible != UINT32_MAX)
        {
            transferIndex = transferCompatible;
            deviceFlags |= PhysicalDeviceFlags_HasTransferQueue;
        }

        const auto checkFlags = [this, deviceFlags](const u16 p_SelectorFlag, const u16 p_DeviceFlag) -> bool {
            return !(m_Flags & p_SelectorFlag) || (deviceFlags & p_DeviceFlag);
        };

        if (!checkFlags(PhysicalDeviceSelectorFlags_RequireGraphicsQueue, PhysicalDeviceFlags_HasGraphicsQueue))
            continue;
        if (!checkFlags(PhysicalDeviceSelectorFlags_RequireComputeQueue, PhysicalDeviceFlags_HasComputeQueue))
            continue;
        if (!checkFlags(PhysicalDeviceSelectorFlags_RequireTransferQueue, PhysicalDeviceFlags_HasTransferQueue))
            continue;
        if (!checkFlags(PhysicalDeviceSelectorFlags_RequirePresentQueue, PhysicalDeviceFlags_HasPresentQueue))
            continue;

        if (!checkFlags(PhysicalDeviceSelectorFlags_DedicatedComputeQueue, PhysicalDeviceFlags_DedicatedComputeQueue))
            continue;
        if (!checkFlags(PhysicalDeviceSelectorFlags_DedicatedTransferQueue, PhysicalDeviceFlags_DedicatedTransferQueue))
            continue;
        if (!checkFlags(PhysicalDeviceSelectorFlags_SeparateComputeQueue, PhysicalDeviceFlags_SeparateComputeQueue))
            continue;
        if (!checkFlags(PhysicalDeviceSelectorFlags_SeparateTransferQueue, PhysicalDeviceFlags_SeparateTransferQueue))
            continue;

        if (checkFlag(PhysicalDeviceSelectorFlags_RequirePresentQueue))
        {
            const auto querySurfaceFormats = m_Instance.GetFunction<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
                "vkGetPhysicalDeviceSurfaceFormatsKHR");
            const auto queryPresentModes = m_Instance.GetFunction<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
                "vkGetPhysicalDeviceSurfacePresentModesKHR");
            if (!querySurfaceFormats || !queryPresentModes)
                continue;

            u32 formatCount = 0;
            u32 modeCount = 0;

            const VkResult result1 = querySurfaceFormats(vkdevice, m_Surface, &formatCount, nullptr);
            const VkResult result2 = queryPresentModes(vkdevice, m_Surface, &modeCount, nullptr);
            if (result1 != VK_SUCCESS || result2 != VK_SUCCESS || formatCount == 0 || modeCount == 0)
                continue;
        }

        const bool v11 = instanceInfo.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 1, 0);
        const bool prop2 = instanceInfo.Flags & InstanceFlags_Properties2Extension;

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
                getFeatures2 = m_Instance.GetFunction<PFN_vkGetPhysicalDeviceFeatures2>("vkGetPhysicalDeviceFeatures2");
                getProperties2 =
                    m_Instance.GetFunction<PFN_vkGetPhysicalDeviceProperties2>("vkGetPhysicalDeviceProperties2");
            }
            else
            {
                getFeatures2 =
                    m_Instance.GetFunction<PFN_vkGetPhysicalDeviceFeatures2KHR>("vkGetPhysicalDeviceFeatures2KHR");
                getProperties2 =
                    m_Instance.GetFunction<PFN_vkGetPhysicalDeviceProperties2KHR>("vkGetPhysicalDeviceProperties2KHR");
            }
            if (!getFeatures2 || !getProperties2)
                return EnumerateResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                              "Failed to get the vkGetPhysicalDeviceFeatures2(KHR) function");

#ifdef VKIT_API_VERSION_1_2
            featuresChain.pNext = &features.Vulkan11;
            propertiesChain.pNext = &properties.Vulkan11;

            features.Vulkan11.pNext = &features.Vulkan12;
            properties.Vulkan11.pNext = &properties.Vulkan12;
#endif
#ifdef VKIT_API_VERSION_1_3
            features.Vulkan12.pNext = &features.Vulkan13;
            properties.Vulkan12.pNext = &properties.Vulkan13;
#endif

            getFeatures2(vkdevice, &featuresChain);
            getProperties2(vkdevice, &propertiesChain);

            features.Core = featuresChain.features;
            properties.Core = propertiesChain.properties;
        }
        else
        {
            getFeatures(vkdevice, &features.Core);
            getProperties(vkdevice, &properties.Core);
        }

        if (!compareFeatureStructs(features.Core, m_RequiredFeatures.Core))
            continue;

#ifdef VKIT_API_VERSION_1_2
        if (!compareFeatureStructs(features.Vulkan11, m_RequiredFeatures.Vulkan11) ||
            !compareFeatureStructs(features.Vulkan12, m_RequiredFeatures.Vulkan12))
            continue;
#endif
#ifdef VKIT_API_VERSION_1_3
        if (!compareFeatureStructs(features.Vulkan13, m_RequiredFeatures.Vulkan13))
            continue;
#endif

        if (m_Name != nullptr && strcmp(m_Name, properties.Core.deviceName) != 0)
            continue;
        if (properties.Core.apiVersion < instanceInfo.ApiVersion)
            continue;
        if (!contains(m_PreferredType, Type(properties.Core.deviceType)))
        {
            if (!checkFlag(PhysicalDeviceSelectorFlags_AnyType))
                continue;
            fullySuitable = false;
        }

        getMemoryProperties(vkdevice, &properties.Memory);
        TKIT_ASSERT(m_RequestedMemory >= m_RequiredMemory,
                    "Requested memory must be greater than or equal to required memory");

        skipDevice = false;
        for (u32 i = 0; i < properties.Memory.memoryHeapCount; ++i)
        {
            if (!(properties.Memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT))
                continue;

            const VkDeviceSize size = properties.Memory.memoryHeaps[i].size;
            if (m_RequiredMemory > 0 && size < m_RequiredMemory)
            {
                skipDevice = true;
                break;
            }
            if (m_RequestedMemory > 0 && size < m_RequestedMemory)
                fullySuitable = false;
        }
        if (skipDevice)
            continue;

        if (fullySuitable)
            deviceFlags |= PhysicalDeviceFlags_Optimal;

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
        devices.emplace_back(vkdevice, deviceInfo);
    }

    std::stable_partition(devices.begin(), devices.end(), [](const PhysicalDevice &p_Device) {
        return p_Device.GetInfo().Flags & PhysicalDeviceFlags_Optimal;
    });
    return EnumerateResult::Ok(devices);
}

PhysicalDevice::PhysicalDevice(VkPhysicalDevice p_Device, const Info &p_Info) noexcept
    : m_Device(p_Device), m_Info(p_Info)
{
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
PhysicalDevice::Selector &PhysicalDevice::Selector::SetFlags(const u16 p_Flags) noexcept
{
    m_Flags = p_Flags;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::AddFlags(const u16 p_Flags) noexcept
{
    m_Flags |= p_Flags;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RemoveFlags(const u16 p_Flags) noexcept
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