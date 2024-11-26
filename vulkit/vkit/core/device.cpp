#include "vkit/core/pch.hpp"
#include "vkit/core/device.hpp"
#include "vkit/core/core.hpp"
#include "tkit/core/logging.hpp"

namespace VKit
{
#ifdef TKIT_OS_APPLE
static const std::array<const char *, 2> s_DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                               "VK_KHR_portability_subset"};
#else
static const std::array<const char *, 1> s_DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#endif

// I could use a stack-allocated buffer for the majority of the dynamic allocations there are. But this is a one-time
// initialization setup. It is not worth the effort

static bool checkDeviceExtensionSupport(const VkPhysicalDevice p_Device) noexcept
{
    u32 extensionCount;
    vkEnumerateDeviceExtensionProperties(p_Device, nullptr, &extensionCount, nullptr);

    DynamicArray<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(p_Device, nullptr, &extensionCount, extensions.data());

    // TKIT_LOG_INFO("Available device extensions:");
    HashSet<std::string> availableExtensions;
    for (const auto &extension : extensions)
    {
        // TKIT_LOG_INFO("  {}", extension.extensionName);
        availableExtensions.insert(extension.extensionName);
    }

    // TKIT_LOG_INFO("Required device extensions:");
    bool contained = true;
    for (const auto &extension : s_DeviceExtensions)
    {
        // TKIT_LOG_INFO("  {}", extension);
        contained &= availableExtensions.contains(extension);
    }

    return contained;
}

static Device::QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice p_Device,
                                                    const VkSurfaceKHR p_Surface) noexcept
{
    Device::QueueFamilyIndices indices;

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(p_Device, &queueFamilyCount, nullptr);

    DynamicArray<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(p_Device, &queueFamilyCount, queueFamilies.data());

    for (u32 i = 0; i < queueFamilies.size(); ++i)
    {
        if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.GraphicsFamily = i;
            indices.GraphicsFamilyHasValue = true;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(p_Device, i, p_Surface, &presentSupport);
        if (queueFamilies[i].queueCount > 0 && presentSupport)
        {
            indices.PresentFamily = i;
            indices.PresentFamilyHasValue = true;
        }
        if (indices.PresentFamilyHasValue && indices.GraphicsFamilyHasValue)
            break;
    }

    return indices;
}

static Device::SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice p_Device,
                                                             const VkSurfaceKHR p_Surface) noexcept
{
    Device::SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_Device, p_Surface, &details.Capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(p_Device, p_Surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(p_Device, p_Surface, &formatCount, details.Formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(p_Device, p_Surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(p_Device, p_Surface, &presentModeCount, details.PresentModes.data());
    }

    return details;
}

static bool isDeviceSuitable(const VkPhysicalDevice p_Device, const VkSurfaceKHR p_Surface) noexcept
{
    const Device::QueueFamilyIndices indices = findQueueFamilies(p_Device, p_Surface);
    const bool extensionsSupported = checkDeviceExtensionSupport(p_Device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        const Device::SwapChainSupportDetails swapChainSupport = querySwapChainSupport(p_Device, p_Surface);
        swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(p_Device, &supportedFeatures);

    return indices.PresentFamilyHasValue && indices.GraphicsFamilyHasValue && extensionsSupported &&
           swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

Device::Device(const VkSurfaceKHR p_Surface) noexcept
{
    TKIT_LOG_INFO("Attempting to create a new device...");
    m_Instance = Core::GetInstance();
    TKIT_ASSERT(m_Instance, "The Vulkit library must be initialized before creating a device");

    pickPhysicalDevice(p_Surface);
    createLogicalDevice(p_Surface);
    createCommandPool(p_Surface);

#ifdef TKIT_ENABLE_VULKAN_PROFILING
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(m_Device, &allocInfo, &m_ProfilingCommandBuffer);
    m_ProfilingContext =
        TKIT_PROFILE_CREATE_VULKAN_CONTEXT(m_PhysicalDevice, m_Device, m_GraphicsQueue, m_ProfilingCommandBuffer);
#endif
}

Device::~Device() noexcept
{
#ifdef TKIT_ENABLE_VULKAN_PROFILING
    TKIT_PROFILE_DESTROY_VULKAN_CONTEXT(m_ProfilingContext);
    vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &m_ProfilingCommandBuffer);
#endif
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    vkDestroyDevice(m_Device, nullptr);
}

void Device::WaitIdle() noexcept
{
    LockQueues();
    vkDeviceWaitIdle(m_Device);
    UnlockQueues();
}

bool Device::IsSuitable(const VkSurfaceKHR p_Surface) const noexcept
{
    return isDeviceSuitable(m_PhysicalDevice, p_Surface);
}

VkDevice Device::GetDevice() const noexcept
{
    return m_Device;
}
VkPhysicalDevice Device::GetPhysicalDevice() const noexcept
{
    return m_PhysicalDevice;
}

Device::QueueFamilyIndices Device::FindQueueFamilies(const VkSurfaceKHR p_Surface) const noexcept
{
    return findQueueFamilies(m_PhysicalDevice, p_Surface);
}
Device::SwapChainSupportDetails Device::QuerySwapChainSupport(const VkSurfaceKHR p_Surface) const noexcept
{
    return querySwapChainSupport(m_PhysicalDevice, p_Surface);
}

u32 Device::FindMemoryType(const u32 p_TypeFilter, const VkMemoryPropertyFlags p_Properties) const noexcept
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i)
        if ((p_TypeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & p_Properties) == p_Properties)
            return i;

    TKIT_ERROR("Failed to find suitable memory type");
    return UINT32_MAX;
}

VkQueue Device::GetGraphicsQueue() const noexcept
{
    return m_GraphicsQueue;
}
VkQueue Device::GetPresentQueue() const noexcept
{
    return m_PresentQueue;
}

const VkPhysicalDeviceProperties &Device::GetProperties() const noexcept
{
    return m_Properties;
}

std::mutex &Device::GraphicsMutex() noexcept
{
    return m_GraphicsMutex;
}

std::mutex &Device::PresentMutex() noexcept
{
    return m_GraphicsQueue == m_PresentQueue ? m_GraphicsMutex : m_PresentMutex;
}

void Device::LockQueues() noexcept
{
    if (m_GraphicsQueue != m_PresentQueue)
        std::lock(m_GraphicsMutex, m_PresentMutex);
    else
        m_GraphicsMutex.lock();
}

void Device::UnlockQueues() noexcept
{
    if (m_GraphicsQueue != m_PresentQueue)
    {
        m_GraphicsMutex.unlock();
        m_PresentMutex.unlock();
    }
    else
        m_GraphicsMutex.unlock();
}

VkCommandBuffer Device::BeginSingleTimeCommands(const VkCommandPool p_Pool) const noexcept
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = p_Pool ? p_Pool : m_CommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    TKIT_ASSERT_RETURNS(vkBeginCommandBuffer(commandBuffer, &beginInfo), VK_SUCCESS, "Failed to begin command buffer");

    return commandBuffer;
}

void Device::EndSingleTimeCommands(const VkCommandBuffer p_CommandBuffer, const VkCommandPool p_Pool) const noexcept
{
    vkEndCommandBuffer(p_CommandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &p_CommandBuffer;

    vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_GraphicsQueue);

    vkFreeCommandBuffers(m_Device, p_Pool ? p_Pool : m_CommandPool, 1, &p_CommandBuffer);
}

void Device::pickPhysicalDevice(const VkSurfaceKHR p_Surface) noexcept
{
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance->GetInstance(), &deviceCount, nullptr);
    TKIT_ASSERT(deviceCount != 0, "Failed to find GPUs with Vulkan support");

    TKIT_LOG_INFO("Device count: {}", deviceCount);
    DynamicArray<VkPhysicalDevice> devices(deviceCount);

    vkEnumeratePhysicalDevices(m_Instance->GetInstance(), &deviceCount, devices.data());

    for (const VkPhysicalDevice device : devices)
        if (isDeviceSuitable(device, p_Surface))
        {
            m_PhysicalDevice = device;
            break;
        }

    TKIT_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU");

    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_Properties);
    TKIT_LOG_INFO("Physical device: {}", m_Properties.deviceName);
}

void Device::createLogicalDevice(const VkSurfaceKHR p_Surface) noexcept
{
    const QueueFamilyIndices indices = FindQueueFamilies(p_Surface);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    const std::unordered_set<std::uint32_t> uniqueQueueFamily = {indices.GraphicsFamily, indices.PresentFamily};

    const f32 queuePriority = 1.0f;
    for (std::uint32_t queue_family : uniqueQueueFamily)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queue_family;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &device_features;
    createInfo.enabledExtensionCount = static_cast<u32>(s_DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();

    // might not really be necessary anymore because device specific validation layers
    // have been deprecated
#ifdef TKIT_ENABLE_ASSERTS
    const char *vLayer = Instance::GetValidationLayer();
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = &vLayer;
#else
    createInfo.enabledLayerCount = 0;
#endif

    TKIT_ASSERT_RETURNS(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device), VK_SUCCESS,
                        "Failed to create logical device");

    vkGetDeviceQueue(m_Device, indices.GraphicsFamily, 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, indices.PresentFamily, 0, &m_PresentQueue);
}

void Device::createCommandPool(const VkSurfaceKHR p_Surface) noexcept
{
    const Device::QueueFamilyIndices indices = FindQueueFamilies(p_Surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = indices.GraphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    TKIT_ASSERT_RETURNS(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool), VK_SUCCESS,
                        "Failed to create command pool");
}

VkFormat Device::FindSupportedFormat(const std::span<const VkFormat> p_Candidates, const VkImageTiling p_Tiling,
                                     const VkFormatFeatureFlags p_Features) const noexcept
{
    for (const VkFormat candidate : p_Candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, candidate, &props);

        if (p_Tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & p_Features) == p_Features)
            return candidate;
        if (p_Tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & p_Features) == p_Features)
            return candidate;
    }

    TKIT_ASSERT(false, "Failed to find a supported format");
    return VK_FORMAT_UNDEFINED;
}

#ifdef TKIT_ENABLE_VULKAN_PROFILING
TKit::VkProfilingContext Device::GetProfilingContext() const noexcept
{
    return m_ProfilingContext;
}
#endif

} // namespace VKit