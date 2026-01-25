#include "vkit/core/pch.hpp"
#include "vkit/device/physical_device.hpp"
#include "vkit/execution/queue.hpp"
#include "tkit/container/stack_array.hpp"

namespace VKit
{

static constexpr TKit::FixedArray<VkBool32 VkPhysicalDeviceFeatures::*, 55> s_CoreMembers = {
    &VkPhysicalDeviceFeatures::robustBufferAccess,
    &VkPhysicalDeviceFeatures::fullDrawIndexUint32,
    &VkPhysicalDeviceFeatures::imageCubeArray,
    &VkPhysicalDeviceFeatures::independentBlend,
    &VkPhysicalDeviceFeatures::geometryShader,
    &VkPhysicalDeviceFeatures::tessellationShader,
    &VkPhysicalDeviceFeatures::sampleRateShading,
    &VkPhysicalDeviceFeatures::dualSrcBlend,
    &VkPhysicalDeviceFeatures::logicOp,
    &VkPhysicalDeviceFeatures::multiDrawIndirect,
    &VkPhysicalDeviceFeatures::drawIndirectFirstInstance,
    &VkPhysicalDeviceFeatures::depthClamp,
    &VkPhysicalDeviceFeatures::depthBiasClamp,
    &VkPhysicalDeviceFeatures::fillModeNonSolid,
    &VkPhysicalDeviceFeatures::depthBounds,
    &VkPhysicalDeviceFeatures::wideLines,
    &VkPhysicalDeviceFeatures::largePoints,
    &VkPhysicalDeviceFeatures::alphaToOne,
    &VkPhysicalDeviceFeatures::multiViewport,
    &VkPhysicalDeviceFeatures::samplerAnisotropy,
    &VkPhysicalDeviceFeatures::textureCompressionETC2,
    &VkPhysicalDeviceFeatures::textureCompressionASTC_LDR,
    &VkPhysicalDeviceFeatures::textureCompressionBC,
    &VkPhysicalDeviceFeatures::occlusionQueryPrecise,
    &VkPhysicalDeviceFeatures::pipelineStatisticsQuery,
    &VkPhysicalDeviceFeatures::vertexPipelineStoresAndAtomics,
    &VkPhysicalDeviceFeatures::fragmentStoresAndAtomics,
    &VkPhysicalDeviceFeatures::shaderTessellationAndGeometryPointSize,
    &VkPhysicalDeviceFeatures::shaderImageGatherExtended,
    &VkPhysicalDeviceFeatures::shaderStorageImageExtendedFormats,
    &VkPhysicalDeviceFeatures::shaderStorageImageMultisample,
    &VkPhysicalDeviceFeatures::shaderStorageImageReadWithoutFormat,
    &VkPhysicalDeviceFeatures::shaderStorageImageWriteWithoutFormat,
    &VkPhysicalDeviceFeatures::shaderUniformBufferArrayDynamicIndexing,
    &VkPhysicalDeviceFeatures::shaderSampledImageArrayDynamicIndexing,
    &VkPhysicalDeviceFeatures::shaderStorageBufferArrayDynamicIndexing,
    &VkPhysicalDeviceFeatures::shaderStorageImageArrayDynamicIndexing,
    &VkPhysicalDeviceFeatures::shaderClipDistance,
    &VkPhysicalDeviceFeatures::shaderCullDistance,
    &VkPhysicalDeviceFeatures::shaderFloat64,
    &VkPhysicalDeviceFeatures::shaderInt64,
    &VkPhysicalDeviceFeatures::shaderInt16,
    &VkPhysicalDeviceFeatures::shaderResourceResidency,
    &VkPhysicalDeviceFeatures::shaderResourceMinLod,
    &VkPhysicalDeviceFeatures::sparseBinding,
    &VkPhysicalDeviceFeatures::sparseResidencyBuffer,
    &VkPhysicalDeviceFeatures::sparseResidencyImage2D,
    &VkPhysicalDeviceFeatures::sparseResidencyImage3D,
    &VkPhysicalDeviceFeatures::sparseResidency2Samples,
    &VkPhysicalDeviceFeatures::sparseResidency4Samples,
    &VkPhysicalDeviceFeatures::sparseResidency8Samples,
    &VkPhysicalDeviceFeatures::sparseResidency16Samples,
    &VkPhysicalDeviceFeatures::sparseResidencyAliased,
    &VkPhysicalDeviceFeatures::variableMultisampleRate,
    &VkPhysicalDeviceFeatures::inheritedQueries};

#ifdef VKIT_API_VERSION_1_2
static constexpr TKit::FixedArray<VkBool32 VkPhysicalDeviceVulkan11Features::*, 12> s_Vulkan11Members = {
    &VkPhysicalDeviceVulkan11Features::storageBuffer16BitAccess,
    &VkPhysicalDeviceVulkan11Features::uniformAndStorageBuffer16BitAccess,
    &VkPhysicalDeviceVulkan11Features::storagePushConstant16,
    &VkPhysicalDeviceVulkan11Features::storageInputOutput16,
    &VkPhysicalDeviceVulkan11Features::multiview,
    &VkPhysicalDeviceVulkan11Features::multiviewGeometryShader,
    &VkPhysicalDeviceVulkan11Features::multiviewTessellationShader,
    &VkPhysicalDeviceVulkan11Features::variablePointersStorageBuffer,
    &VkPhysicalDeviceVulkan11Features::variablePointers,
    &VkPhysicalDeviceVulkan11Features::protectedMemory,
    &VkPhysicalDeviceVulkan11Features::samplerYcbcrConversion,
    &VkPhysicalDeviceVulkan11Features::shaderDrawParameters};

static constexpr TKit::FixedArray<VkBool32 VkPhysicalDeviceVulkan12Features::*, 47> s_Vulkan12Members = {
    &VkPhysicalDeviceVulkan12Features::samplerMirrorClampToEdge,
    &VkPhysicalDeviceVulkan12Features::drawIndirectCount,
    &VkPhysicalDeviceVulkan12Features::storageBuffer8BitAccess,
    &VkPhysicalDeviceVulkan12Features::uniformAndStorageBuffer8BitAccess,
    &VkPhysicalDeviceVulkan12Features::storagePushConstant8,
    &VkPhysicalDeviceVulkan12Features::shaderBufferInt64Atomics,
    &VkPhysicalDeviceVulkan12Features::shaderSharedInt64Atomics,
    &VkPhysicalDeviceVulkan12Features::shaderFloat16,
    &VkPhysicalDeviceVulkan12Features::shaderInt8,
    &VkPhysicalDeviceVulkan12Features::descriptorIndexing,
    &VkPhysicalDeviceVulkan12Features::shaderInputAttachmentArrayDynamicIndexing,
    &VkPhysicalDeviceVulkan12Features::shaderUniformTexelBufferArrayDynamicIndexing,
    &VkPhysicalDeviceVulkan12Features::shaderStorageTexelBufferArrayDynamicIndexing,
    &VkPhysicalDeviceVulkan12Features::shaderUniformBufferArrayNonUniformIndexing,
    &VkPhysicalDeviceVulkan12Features::shaderSampledImageArrayNonUniformIndexing,
    &VkPhysicalDeviceVulkan12Features::shaderStorageBufferArrayNonUniformIndexing,
    &VkPhysicalDeviceVulkan12Features::shaderStorageImageArrayNonUniformIndexing,
    &VkPhysicalDeviceVulkan12Features::shaderInputAttachmentArrayNonUniformIndexing,
    &VkPhysicalDeviceVulkan12Features::shaderUniformTexelBufferArrayNonUniformIndexing,
    &VkPhysicalDeviceVulkan12Features::shaderStorageTexelBufferArrayNonUniformIndexing,
    &VkPhysicalDeviceVulkan12Features::descriptorBindingUniformBufferUpdateAfterBind,
    &VkPhysicalDeviceVulkan12Features::descriptorBindingSampledImageUpdateAfterBind,
    &VkPhysicalDeviceVulkan12Features::descriptorBindingStorageImageUpdateAfterBind,
    &VkPhysicalDeviceVulkan12Features::descriptorBindingStorageBufferUpdateAfterBind,
    &VkPhysicalDeviceVulkan12Features::descriptorBindingUniformTexelBufferUpdateAfterBind,
    &VkPhysicalDeviceVulkan12Features::descriptorBindingStorageTexelBufferUpdateAfterBind,
    &VkPhysicalDeviceVulkan12Features::descriptorBindingUpdateUnusedWhilePending,
    &VkPhysicalDeviceVulkan12Features::descriptorBindingPartiallyBound,
    &VkPhysicalDeviceVulkan12Features::descriptorBindingVariableDescriptorCount,
    &VkPhysicalDeviceVulkan12Features::runtimeDescriptorArray,
    &VkPhysicalDeviceVulkan12Features::samplerFilterMinmax,
    &VkPhysicalDeviceVulkan12Features::scalarBlockLayout,
    &VkPhysicalDeviceVulkan12Features::imagelessFramebuffer,
    &VkPhysicalDeviceVulkan12Features::uniformBufferStandardLayout,
    &VkPhysicalDeviceVulkan12Features::shaderSubgroupExtendedTypes,
    &VkPhysicalDeviceVulkan12Features::separateDepthStencilLayouts,
    &VkPhysicalDeviceVulkan12Features::hostQueryReset,
    &VkPhysicalDeviceVulkan12Features::timelineSemaphore,
    &VkPhysicalDeviceVulkan12Features::bufferDeviceAddress,
    &VkPhysicalDeviceVulkan12Features::bufferDeviceAddressCaptureReplay,
    &VkPhysicalDeviceVulkan12Features::bufferDeviceAddressMultiDevice,
    &VkPhysicalDeviceVulkan12Features::vulkanMemoryModel,
    &VkPhysicalDeviceVulkan12Features::vulkanMemoryModelDeviceScope,
    &VkPhysicalDeviceVulkan12Features::vulkanMemoryModelAvailabilityVisibilityChains,
    &VkPhysicalDeviceVulkan12Features::shaderOutputViewportIndex,
    &VkPhysicalDeviceVulkan12Features::shaderOutputLayer,
    &VkPhysicalDeviceVulkan12Features::subgroupBroadcastDynamicId};
#endif

#ifdef VKIT_API_VERSION_1_3
static constexpr TKit::FixedArray<VkBool32 VkPhysicalDeviceVulkan13Features::*, 15> s_Vulkan13Members = {
    &VkPhysicalDeviceVulkan13Features::robustImageAccess,
    &VkPhysicalDeviceVulkan13Features::inlineUniformBlock,
    &VkPhysicalDeviceVulkan13Features::descriptorBindingInlineUniformBlockUpdateAfterBind,
    &VkPhysicalDeviceVulkan13Features::pipelineCreationCacheControl,
    &VkPhysicalDeviceVulkan13Features::privateData,
    &VkPhysicalDeviceVulkan13Features::shaderDemoteToHelperInvocation,
    &VkPhysicalDeviceVulkan13Features::shaderTerminateInvocation,
    &VkPhysicalDeviceVulkan13Features::subgroupSizeControl,
    &VkPhysicalDeviceVulkan13Features::computeFullSubgroups,
    &VkPhysicalDeviceVulkan13Features::synchronization2,
    &VkPhysicalDeviceVulkan13Features::textureCompressionASTC_HDR,
    &VkPhysicalDeviceVulkan13Features::shaderZeroInitializeWorkgroupMemory,
    &VkPhysicalDeviceVulkan13Features::dynamicRendering,
    &VkPhysicalDeviceVulkan13Features::shaderIntegerDotProduct,
    &VkPhysicalDeviceVulkan13Features::maintenance4};
#endif

#ifdef VKIT_API_VERSION_1_4
static constexpr TKit::FixedArray<VkBool32 VkPhysicalDeviceVulkan14Features::*, 21> s_Vulkan14Members = {
    &VkPhysicalDeviceVulkan14Features::globalPriorityQuery,
    &VkPhysicalDeviceVulkan14Features::shaderSubgroupRotate,
    &VkPhysicalDeviceVulkan14Features::shaderSubgroupRotateClustered,
    &VkPhysicalDeviceVulkan14Features::shaderFloatControls2,
    &VkPhysicalDeviceVulkan14Features::shaderExpectAssume,
    &VkPhysicalDeviceVulkan14Features::rectangularLines,
    &VkPhysicalDeviceVulkan14Features::bresenhamLines,
    &VkPhysicalDeviceVulkan14Features::smoothLines,
    &VkPhysicalDeviceVulkan14Features::stippledRectangularLines,
    &VkPhysicalDeviceVulkan14Features::stippledBresenhamLines,
    &VkPhysicalDeviceVulkan14Features::stippledSmoothLines,
    &VkPhysicalDeviceVulkan14Features::vertexAttributeInstanceRateDivisor,
    &VkPhysicalDeviceVulkan14Features::vertexAttributeInstanceRateZeroDivisor,
    &VkPhysicalDeviceVulkan14Features::indexTypeUint8,
    &VkPhysicalDeviceVulkan14Features::dynamicRenderingLocalRead,
    &VkPhysicalDeviceVulkan14Features::maintenance5,
    &VkPhysicalDeviceVulkan14Features::maintenance6,
    &VkPhysicalDeviceVulkan14Features::pipelineProtectedAccess,
    &VkPhysicalDeviceVulkan14Features::pipelineRobustness,
    &VkPhysicalDeviceVulkan14Features::hostImageCopy,
    &VkPhysicalDeviceVulkan14Features::pushDescriptor};
#endif

template <typename C, typename T> static bool contains(const C &container, const T &value)
{
    return std::find(container.begin(), container.end(), value) != container.end();
}

template <typename T> const auto &getMembers()
{
#ifdef VKIT_API_VERSION_1_2
    if constexpr (std::is_same_v<T, VkPhysicalDeviceVulkan11Features>)
        return s_Vulkan11Members;
    else if constexpr (std::is_same_v<T, VkPhysicalDeviceVulkan12Features>)
        return s_Vulkan12Members;
#endif
#ifdef VKIT_API_VERSION_1_3
    else if constexpr (std::is_same_v<T, VkPhysicalDeviceVulkan13Features>)
        return s_Vulkan13Members;
#endif
#ifdef VKIT_API_VERSION_1_4
    else if constexpr (std::is_same_v<T, VkPhysicalDeviceVulkan14Features>)
        return s_Vulkan14Members;
#endif
#ifdef VKIT_API_VERSION_1_2
    else
        return s_CoreMembers;
#else
    return s_CoreMembers;
#endif
}

template <typename T> static void orFeatures(T &dst, const T &src)
{
    const auto &members = getMembers<T>();
    for (const auto member : members)
        dst.*member |= src.*member;
}

static void orFeatures(DeviceFeatures &dst, const DeviceFeatures &src)
{
    orFeatures(dst.Core, src.Core);
#ifdef VKIT_API_VERSION_1_2
    orFeatures(dst.Vulkan11, src.Vulkan11);
    orFeatures(dst.Vulkan12, src.Vulkan12);
#endif
#ifdef VKIT_API_VERSION_1_3
    orFeatures(dst.Vulkan13, src.Vulkan13);
#endif
#ifdef VKIT_API_VERSION_1_4
    orFeatures(dst.Vulkan14, src.Vulkan14);
#endif
}

template <typename T> static bool compareFeatureStructs(const T &supported, const T &requested)
{
    const auto &members = getMembers<T>();

    for (const auto member : members)
        if (!(supported.*member) && requested.*member)
            return false;
    return true;
}

static bool compareFeatures(const DeviceFeatures &supported, const DeviceFeatures &requested)
{
    if (!compareFeatureStructs(supported.Core, requested.Core))
        return false;
#ifdef VKIT_API_VERSION_1_2
    if (!compareFeatureStructs(supported.Vulkan11, requested.Vulkan11))
        return false;
    if (!compareFeatureStructs(supported.Vulkan12, requested.Vulkan12))
        return false;
#endif
#ifdef VKIT_API_VERSION_1_3
    if (!compareFeatureStructs(supported.Vulkan13, requested.Vulkan13))
        return false;
#endif
#ifdef VKIT_API_VERSION_1_4
    if (!compareFeatureStructs(supported.Vulkan14, requested.Vulkan14))
        return false;
#endif
    return true;
}

#ifdef VK_KHR_surface
static Result<PhysicalDevice::SwapChainSupportDetails> querySwapChainSupport(const Vulkan::InstanceTable *table,
                                                                             const VkPhysicalDevice device,
                                                                             const VkSurfaceKHR surface)
{
    using Res = Result<PhysicalDevice::SwapChainSupportDetails>;
    u32 formatCount = 0;
    u32 modeCount = 0;

    VkResult result = table->GetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (result != VK_SUCCESS)
        return Res::Error(result);

    result = table->GetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, nullptr);
    if (result != VK_SUCCESS)
        return Res::Error(result);

    if (formatCount == 0 || modeCount == 0)
        return Res::Error(Error_NoSurfaceCapabilities);

    PhysicalDevice::SwapChainSupportDetails details;
    result = table->GetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);
    if (result != VK_SUCCESS)
        return Res::Error(result);

    details.Formats.Resize(formatCount);
    details.PresentModes.Resize(modeCount);

    result = table->GetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.GetData());
    if (result != VK_SUCCESS)
        return Res::Error(result);

    result =
        table->GetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, details.PresentModes.GetData());
    if (result != VK_SUCCESS)
        return Res::Error(result);

    return details;
}
#endif

#if defined(VKIT_API_VERSION_1_1) || defined(VK_KHR_get_physical_device_properties2)
template <typename T2, typename T1> void createChain(T2 &chain, T1 &properties, const u32 apiVersion)
{
#    ifndef VKIT_API_VERSION_1_2
    chain.pNext = properties.Next;
    return;
#    else
    if (apiVersion < VKIT_API_VERSION_1_2)
    {
        chain.pNext = properties.Next;
        return;
    }
    chain.pNext = &properties.Vulkan11;
    properties.Vulkan11.pNext = &properties.Vulkan12;
    properties.Vulkan12.pNext = properties.Next;
#        ifdef VKIT_API_VERSION_1_3
    if (apiVersion >= VKIT_API_VERSION_1_3)
    {
        properties.Vulkan12.pNext = &properties.Vulkan13;
        properties.Vulkan13.pNext = properties.Next;
    }
#            ifdef VKIT_API_VERSION_1_4
    if (apiVersion >= VKIT_API_VERSION_1_4)
    {
        properties.Vulkan13.pNext = &properties.Vulkan14;
        properties.Vulkan14.pNext = properties.Next;
    }
#            endif
#        endif
#    endif
}
VkPhysicalDeviceFeatures2KHR DeviceFeatures::CreateChain(const u32 apiVersion)
{
    VkPhysicalDeviceFeatures2KHR features{};
    features.features = Core;
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
    createChain<VkPhysicalDeviceFeatures2KHR>(features, *this, apiVersion);
    return features;
}

VkPhysicalDeviceProperties2KHR DeviceProperties::CreateChain(const u32 apiVersion)
{
    VkPhysicalDeviceProperties2KHR properties{};
    properties.properties = Core;
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
    createChain<VkPhysicalDeviceProperties2KHR>(properties, *this, apiVersion);
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

Result<PhysicalDevice> PhysicalDevice::Selector::judgeDevice(const VkPhysicalDevice device) const
{
    using JudgeResult = Result<PhysicalDevice>;
    const Instance::Info &instanceInfo = m_Instance->GetInfo();
    const Vulkan::InstanceTable *table = instanceInfo.Table;

    VkPhysicalDeviceProperties quickProperties;
    table->GetPhysicalDeviceProperties(device, &quickProperties);
    const char *name = quickProperties.deviceName;

    if (m_Name && strcmp(m_Name, name) != 0)
        return JudgeResult::Error(
            Error_RejectedDevice,
            TKit::Format("[VULKIT][P-DEVICE] The device name '{}' does not match the requested name '{}'", name,
                         m_Name));

    TKIT_LOG_WARNING_IF(quickProperties.apiVersion < m_RequestedApiVersion,
                        "[VULKIT][P-DEVICE] The device '{}' does not support the requested API version {}.{}.{}", name,
                        VKIT_EXPAND_VERSION(m_RequestedApiVersion));

    if (quickProperties.apiVersion < m_RequiredApiVersion)
        return JudgeResult::Error(
            Error_VersionMismatch,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not support the required API version {}.{}.{}", name,
                         VKIT_EXPAND_VERSION(m_RequiredApiVersion)));

    bool fullySuitable = quickProperties.apiVersion >= m_RequestedApiVersion;

    u32 extensionCount;
    VkResult result = table->EnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    if (result != VK_SUCCESS)
        return JudgeResult::Error(
            result,
            TKit::Format("[VULKIT][P-DEVICE] Failed to get the number of device extensions for the device: {}", name));

    TKit::StackArray<VkExtensionProperties> extensionsProps{extensionCount};
    result = table->EnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensionsProps.GetData());
    if (result != VK_SUCCESS)
        return JudgeResult::Error(
            result, TKit::Format("[VULKIT][P-DEVICE] Failed to get the device extensions for the device: {}", name));

    TKit::StackArray<std::string> availableExtensions;
    availableExtensions.Reserve(extensionsProps.GetSize());
    for (const VkExtensionProperties &extension : extensionsProps)
        availableExtensions.Append(extension.extensionName);

    TKit::StackArray<std::string> enabledExtensions;
    enabledExtensions.Reserve(m_RequestedExtensions.GetCapacity());
    for (const std::string &extension : m_RequiredExtensions)
    {
        if (!contains(availableExtensions, extension))
            return JudgeResult::Error(
                Error_MissingExtension,
                TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not support the required extension '{}'", name,
                             extension));
        enabledExtensions.Append(extension);
    }

    for (const std::string &extension : m_RequestedExtensions)
        if (contains(availableExtensions, extension))
            enabledExtensions.Append(extension);
        else
        {
            TKIT_LOG_WARNING("[VULKIT][P-DEVICE] The device '{}' does not support the requested extension '{}'", name,
                             extension);
            fullySuitable = false;
        }

    DeviceSelectorFlags flags = m_Flags;
    const auto checkFlags = [&flags](const DeviceSelectorFlags pflags) -> bool { return pflags & flags; };

    if (checkFlags(DeviceSelectorFlag_PortabilitySubset) && contains(availableExtensions, "VK_KHR_portability_subset"))
        enabledExtensions.Append("VK_KHR_portability_subset");

    if (checkFlags(DeviceSelectorFlag_RequirePresentQueue))
        enabledExtensions.Append("VK_KHR_swapchain");

    u32 familyCount;
    table->GetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);

    TKit::StackArray<VkQueueFamilyProperties> families{familyCount};
    table->GetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.GetData());

    const auto compatibleQueueIndex = [&](const VkQueueFlags flags) -> u32 {
        for (u32 i = 0; i < familyCount; ++i)
            if (families[i].queueCount > 0 && (families[i].queueFlags & flags) == flags)
                return i;

        return TKIT_U32_MAX;
    };
    const auto dedicatedQueueIndex = [&families, familyCount](const VkQueueFlags flags,
                                                              const VkQueueFlags forbiddenFlags) -> u32 {
        for (u32 i = 0; i < familyCount; ++i)
            if (families[i].queueCount > 0 && (families[i].queueFlags & flags) == flags &&
                !(families[i].queueFlags & forbiddenFlags))
                return i;

        return TKIT_U32_MAX;
    };
    const auto separatedQueueIndex = [&families, familyCount](const VkQueueFlags flags,
                                                              const VkQueueFlags forbiddenFlags) -> u32 {
        u32 index = TKIT_U32_MAX;
        for (u32 i = 0; i < familyCount; ++i)
            if (families[i].queueCount > 0 && (families[i].queueFlags & flags) == flags &&
                !(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                if (!(families[i].queueFlags & forbiddenFlags))
                    return i;
                index = i;
            }
        return index;
    };

#ifdef VK_KHR_surface
    const auto presentQueueIndex = [familyCount, device, &table](const VkSurfaceKHR surface) -> u32 {
        if (!surface || !table->vkGetPhysicalDeviceSurfaceSupportKHR)
            return TKIT_U32_MAX;

        for (u32 i = 0; i < familyCount; ++i)
        {
            VkBool32 presentSupport = VK_FALSE;
            const VkResult result = table->GetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
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

    const auto compareFlags = [&flags, deviceFlags](const DeviceSelectorFlags selectorFlag,
                                                    const DeviceFlags deviceFlag) -> bool {
        return !(flags & selectorFlag) || (deviceFlags & deviceFlag);
    };

    if (!compareFlags(DeviceSelectorFlag_RequireGraphicsQueue, DeviceFlag_HasGraphicsQueue))
        return JudgeResult::Error(
            Error_MissingQueue,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have a graphics queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequireComputeQueue, DeviceFlag_HasComputeQueue))
        return JudgeResult::Error(
            Error_MissingQueue, TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have a compute queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequireTransferQueue, DeviceFlag_HasTransferQueue))
        return JudgeResult::Error(
            Error_MissingQueue,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have a transfer queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequirePresentQueue, DeviceFlag_HasPresentQueue))
        return JudgeResult::Error(
            Error_MissingQueue, TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have a present queue", name));

    if (!compareFlags(DeviceSelectorFlag_RequireDedicatedComputeQueue, DeviceFlag_HasDedicatedComputeQueue))
        return JudgeResult::Error(
            Error_MissingQueue,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have a dedicated compute queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequireDedicatedTransferQueue, DeviceFlag_HasDedicatedTransferQueue))
        return JudgeResult::Error(
            Error_MissingQueue,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have a dedicated transfer queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequireSeparateComputeQueue, DeviceFlag_HasSeparateComputeQueue))
        return JudgeResult::Error(
            Error_MissingQueue,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have a separate compute queue", name));
    if (!compareFlags(DeviceSelectorFlag_RequireSeparateTransferQueue, DeviceFlag_HasSeparateTransferQueue))
        return JudgeResult::Error(
            Error_MissingQueue,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have a separate transfer queue", name));

#ifdef VK_KHR_surface
    if (checkFlags(DeviceSelectorFlag_RequirePresentQueue))
    {
        const auto qresult = querySwapChainSupport(table, device, m_Surface);
        TKIT_RETURN_ON_ERROR(qresult);
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
        return JudgeResult::Error(
            Error_MissingExtension,
            "[VULKIT][P-DEVICE] The 'VK_KHR_get_physical_device_properties2' extension is not supported");
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
            table->GetPhysicalDeviceFeatures2(device, &fchain);
            table->GetPhysicalDeviceProperties2(device, &pchain);
        }
        else
        {
            table->GetPhysicalDeviceFeatures2KHR(device, &fchain);
            table->GetPhysicalDeviceProperties2KHR(device, &pchain);
        }

        features.Core = fchain.features;
        properties.Core = pchain.properties;
    }
    else
    {
        table->GetPhysicalDeviceFeatures(device, &features.Core);
        table->GetPhysicalDeviceProperties(device, &properties.Core);
    }
#else
    table->GetPhysicalDeviceFeatures(device, &features.Core);
    table->GetPhysicalDeviceProperties(device, &properties.Core);
#endif

    if (!compareFeatureStructs(features.Core, m_RequiredFeatures.Core))
        return JudgeResult::Error(
            Error_MissingFeature,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have the required core features", name));

#ifdef VKIT_API_VERSION_1_2
    if (!compareFeatureStructs(features.Vulkan11, m_RequiredFeatures.Vulkan11) ||
        !compareFeatureStructs(features.Vulkan12, m_RequiredFeatures.Vulkan12))
        return JudgeResult::Error(
            Error_MissingFeature,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have the required Vulkan 1.1 or 1.2 features",
                         name));
#endif
#ifdef VKIT_API_VERSION_1_3
    if (!compareFeatureStructs(features.Vulkan13, m_RequiredFeatures.Vulkan13))
        return JudgeResult::Error(
            Error_MissingFeature,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have the required Vulkan 1.3 features", name));
#endif
#ifdef VKIT_API_VERSION_1_4
    if (!compareFeatureStructs(features.Vulkan14, m_RequiredFeatures.Vulkan14))
        return JudgeResult::Error(
            Error_MissingFeature,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have the required Vulkan 1.4 features", name));
#endif
    if (m_PreferredType != DeviceType(properties.Core.deviceType))
    {
        if (!checkFlags(DeviceSelectorFlag_AnyType))
            return JudgeResult::Error(
                Error_RejectedDevice,
                TKit::Format("[VULKIT][P-DEVICE] The device '{}' is not of the preferred type", name));
        fullySuitable = false;
    }

    table->GetPhysicalDeviceMemoryProperties(device, &properties.Memory);

    TKIT_ASSERT(m_RequestedMemory >= m_RequiredMemory,
                "[VULKIT][P-DEVICE] Requested memory ({}) must be greater than or equal to required memory ({})",
                m_RequestedMemory, m_RequiredMemory);

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
            Error_InsufficientMemory,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have device local memory", name));
    TKIT_LOG_WARNING_IF(!hasRequestedMemory,
                        "[VULKIT][P-DEVICE] The device '{}' does not have the requested memory of {} bytes", name,
                        m_RequestedMemory);
    if (!hasRequiredMemory)
        return JudgeResult::Error(
            Error_InsufficientMemory,
            TKit::Format("[VULKIT][P-DEVICE] The device '{}' does not have the required memory of {} bytes", name,
                         m_RequiredMemory));

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

    return JudgeResult::Ok(device, deviceInfo);
}

PhysicalDevice::Selector::Selector(const Instance *instance, const u32 maxExtensions) : m_Instance(instance)
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

    m_RequestedExtensions.Reserve(maxExtensions);
    m_RequiredExtensions.Reserve(maxExtensions);

    if (!(m_Instance->GetInfo().Flags & InstanceFlag_Headless))
        m_Flags |= DeviceSelectorFlag_RequirePresentQueue;
}

Result<TKit::TierArray<Result<PhysicalDevice>>> PhysicalDevice::Selector::Enumerate() const
{
    using EnumerateResult = Result<TKit::TierArray<Result<PhysicalDevice>>>;

#ifdef VK_KHR_surface
    if ((m_Flags & DeviceSelectorFlag_RequirePresentQueue) && !m_Surface)
        return EnumerateResult::Error(
            Error_BadInput,
            "[VULKIT][P-DEVICE] The surface must be set if the instance is not headless (requires present queue)");
#else
    if (m_Flags & DeviceSelectorFlag_RequirePresentQueue)
        return EnumerateResult::Error(
            Error_MissingExtension,
            "[VULKIT][P-DEVICE] The current version of the vulkan headers does not provide the compile-time "
            "capabilities to enable surface creation. The instance must be headless");
#endif

    TKit::StackArray<VkPhysicalDevice> vkdevices;

    const Vulkan::InstanceTable *table = m_Instance->GetInfo().Table;

    u32 deviceCount = 0;
    VkResult result = table->EnumeratePhysicalDevices(m_Instance->GetHandle(), &deviceCount, nullptr);
    if (result != VK_SUCCESS)
        return EnumerateResult::Error(result);

    vkdevices.Resize(deviceCount);
    result = table->EnumeratePhysicalDevices(m_Instance->GetHandle(), &deviceCount, vkdevices.GetData());
    if (result != VK_SUCCESS)
        return EnumerateResult::Error(result);

    if (vkdevices.IsEmpty())
        return EnumerateResult::Error(Error_NoDeviceFound);

    TKit::TierArray<Result<PhysicalDevice>> devices;
    devices.Reserve(vkdevices.GetSize());
    for (const VkPhysicalDevice vkdevice : vkdevices)
    {
        const auto judgeResult = judgeDevice(vkdevice);
        devices.Append(judgeResult);
    }

    std::stable_partition(devices.begin(), devices.end(), [](const Result<PhysicalDevice> &device) {
        return device && (device.GetValue().GetInfo().Flags & DeviceFlag_Optimal);
    });
    return devices;
}

bool PhysicalDevice::AreFeaturesSupported(const DeviceFeatures &features) const
{
    return compareFeatures(m_Info.AvailableFeatures, features);
}
bool PhysicalDevice::AreFeaturesEnabled(const DeviceFeatures &features) const
{
    return compareFeatures(m_Info.EnabledFeatures, features);
}
bool PhysicalDevice::EnableFeatures(const DeviceFeatures &features)
{
    if (!AreFeaturesSupported(features))
        return false;

    orFeatures(m_Info.EnabledFeatures, features);
    return true;
}

bool PhysicalDevice::IsExtensionSupported(const char *extension) const
{
    return contains(m_Info.AvailableExtensions, extension);
}
bool PhysicalDevice::IsExtensionEnabled(const char *extension) const
{
    return contains(m_Info.EnabledExtensions, extension);
}
bool PhysicalDevice::EnableExtension(const char *extension)
{
    if (IsExtensionEnabled(extension))
        return true;
    if (!IsExtensionSupported(extension))
        return false;
    m_Info.EnabledExtensions.Append(extension);
    return true;
}

#ifdef VK_KHR_surface
Result<PhysicalDevice::SwapChainSupportDetails> PhysicalDevice::QuerySwapChainSupport(const Instance::Proxy &instance,
                                                                                      const VkSurfaceKHR surface) const
{
    return querySwapChainSupport(instance.Table, m_Device, surface);
}
#endif

PhysicalDevice::Selector &PhysicalDevice::Selector::SetName(const char *name)
{
    m_Name = name;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::PreferType(const DeviceType type)
{
    m_PreferredType = type;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireApiVersion(u32 version)
{
    m_RequiredApiVersion = version;
    if (m_RequestedApiVersion < m_RequiredApiVersion)
        m_RequestedApiVersion = m_RequiredApiVersion;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireApiVersion(u32 major, u32 minor, u32 patch)
{
    return RequireApiVersion(VKIT_MAKE_VERSION(0, major, minor, patch));
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestApiVersion(u32 version)
{
    m_RequestedApiVersion = version;
    if (m_RequestedApiVersion < m_RequiredApiVersion)
        m_RequiredApiVersion = m_RequestedApiVersion;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestApiVersion(u32 major, u32 minor, u32 patch)
{
    return RequestApiVersion(VKIT_MAKE_VERSION(0, major, minor, patch));
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireExtension(const char *extension)
{
    m_RequiredExtensions.Append(extension);
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestExtension(const char *extension)
{
    m_RequestedExtensions.Append(extension);
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireMemory(const VkDeviceSize size)
{
    m_RequiredMemory = size;
    if (m_RequestedMemory < m_RequiredMemory)
        m_RequestedMemory = m_RequiredMemory;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequestMemory(const VkDeviceSize size)
{
    m_RequestedMemory = size;
    if (m_RequestedMemory < m_RequiredMemory)
        m_RequiredMemory = m_RequestedMemory;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RequireFeatures(const DeviceFeatures &features)
{
    // void *next = m_RequiredFeatures.Next;
    m_RequiredFeatures = features;
    // m_RequiredFeatures.Next = next;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::SetFlags(const DeviceSelectorFlags flags)
{
    m_Flags = flags;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::AddFlags(const DeviceSelectorFlags flags)
{
    m_Flags |= flags;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::RemoveFlags(const DeviceSelectorFlags flags)
{
    m_Flags &= ~flags;
    return *this;
}
PhysicalDevice::Selector &PhysicalDevice::Selector::SetSurface(const VkSurfaceKHR surface)
{
    m_Surface = surface;
    return *this;
}

} // namespace VKit
