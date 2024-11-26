#include "vkit/core/pch.hpp"
#include "vkit/core/instance.hpp"
#include "vkit/core/alias.hpp"
#include "tkit/core/logging.hpp"

namespace VKit
{
#ifdef VKIT_VALIDATION_LAYERS

static const char *s_ValidationLayer = "VK_LAYER_KHRONOS_validation";
static VkDebugUtilsMessengerEXT s_DebugMessenger;

static const char *toString(VkDebugUtilsMessageSeverityFlagBitsEXT p_Severity)
{
    switch (p_Severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        return "VERBOSE";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        return "ERROR";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        return "WARNING";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        return "INFO";
    default:
        return "UNKNOWN";
    }
}
static const char *toString(VkDebugUtilsMessageTypeFlagsEXT p_MessageType)
{
    if (p_MessageType == 7)
        return "General | Validation | Performance";
    if (p_MessageType == 6)
        return "Validation | Performance";
    if (p_MessageType == 5)
        return "General | Performance";
    if (p_MessageType == 4)
        return "Performance";
    if (p_MessageType == 3)
        return "General | Validation";
    if (p_MessageType == 2)
        return "Validation";
    if (p_MessageType == 1)
        return "General";
    return "Unknown";
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT p_Severity,
                                                    VkDebugUtilsMessageTypeFlagsEXT p_MessageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *p_CallbackData, void *)
{
    TKIT_ERROR("<{}: {}> {}", toString(p_Severity), toString(p_MessageType), p_CallbackData->pMessage);
    return VK_FALSE;
}

static VkResult createDebugUtilsMessengerEXT(const VkInstance p_Instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT *p_CreateInfo,
                                             const VkAllocationCallbacks *p_Allocator,
                                             VkDebugUtilsMessengerEXT *p_DebugMessenger) noexcept
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(p_Instance, "vkCreateDebugUtilsMessengerEXT");
    if (func)
        return func(p_Instance, p_CreateInfo, p_Allocator, p_DebugMessenger);
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void destroyDebugUtilsMessengerEXT(VkInstance p_Instance, const VkAllocationCallbacks *p_Allocator) noexcept
{
    auto func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(p_Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func)
        func(p_Instance, s_DebugMessenger, p_Allocator);
}

static VkDebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo(
    const PFN_vkDebugUtilsMessengerCallbackEXT p_DebugCallback) noexcept
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = p_DebugCallback ? p_DebugCallback : debugCallback;
    createInfo.pUserData = nullptr; // Optional
    return createInfo;
}

static bool checkValidationLayerSupport() noexcept
{
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    DynamicArray<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto &layer_props : availableLayers)
        if (strcmp(s_ValidationLayer, layer_props.layerName) == 0)
            return true;

    return false;
}

static void hasRequiredInstanceExtensions(const DynamicArray<const char *> &p_RequiredExtensions) noexcept
{
    u32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    DynamicArray<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    TKIT_LOG_INFO("Available instance extensions:");
    HashSet<std::string> available;

    for (const auto &extension : extensions)
    {
        TKIT_LOG_INFO("  {}", extension.extensionName);
        available.emplace(extension.extensionName);
    }

    TKIT_LOG_INFO("Required instance extensions:");
    for (const char *required : p_RequiredExtensions)
    {
        TKIT_LOG_INFO("  {}", required);
        TKIT_ASSERT(available.contains(required), "Missing required glfw extension");
    }
}

static void setupDebugMessenger(const VkInstance p_Instance,
                                const PFN_vkDebugUtilsMessengerCallbackEXT p_DebugCallback) noexcept
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = createDebugMessengerCreateInfo(p_DebugCallback);
    TKIT_ASSERT_RETURNS(createDebugUtilsMessengerEXT(p_Instance, &createInfo, nullptr, &s_DebugMessenger), VK_SUCCESS,
                        "Failed to set up debug messenger");
}

const char *Instance::GetValidationLayer() noexcept
{
    return s_ValidationLayer;
}

#endif

/*INSTANCE IMPLEMENTATION*/

Instance::Instance(const Specs &p_Specs) noexcept
{
    createInstance(p_Specs);
#ifdef VKIT_VALIDATION_LAYERS
    setupDebugMessenger(m_Instance, p_Specs.DebugCallback);
#endif
}

Instance::~Instance() noexcept
{
#ifdef VKIT_VALIDATION_LAYERS
    destroyDebugUtilsMessengerEXT(m_Instance, nullptr);
#endif
    vkDestroyInstance(m_Instance, nullptr);
}

VkInstance Instance::GetInstance() const noexcept
{
    return m_Instance;
}

void Instance::createInstance(const Specs &p_Specs) noexcept
{
    TKIT_LOG_INFO("Creating a vulkan instance...");
    TKIT_ASSERT(checkValidationLayerSupport(), "Validation layers requested, but not available!");
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = p_Specs.ApplicationName;
    appInfo.applicationVersion = p_Specs.ApplicationVersion;
    appInfo.pEngineName = p_Specs.EngineName;
    appInfo.engineVersion = p_Specs.EngineVersion;
    appInfo.apiVersion = p_Specs.ApiVersion;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    DynamicArray<const char *> extensions{p_Specs.RequiredExtensions.begin(), p_Specs.RequiredExtensions.end()};
#ifdef VKIT_VALIDATION_LAYERS
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.flags |= p_Specs.Flags;

#ifdef VKIT_VALIDATION_LAYERS
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = &s_ValidationLayer;

    auto dbgCreateInfo = createDebugMessengerCreateInfo(p_Specs.DebugCallback);
    createInfo.pNext = &dbgCreateInfo;
#else
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
#endif

    TKIT_ASSERT_RETURNS(vkCreateInstance(&createInfo, nullptr, &m_Instance), VK_SUCCESS,
                        "Failed to create vulkan instance");
#ifdef VKIT_VALIDATION_LAYERS
    hasRequiredInstanceExtensions(extensions);
#endif
}

} // namespace VKit