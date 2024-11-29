#include "vkit/core/pch.hpp"
#include "vkit/backend/instance.hpp"
#include "vkit/core/alias.hpp"
#include "tkit/core/logging.hpp"

namespace VKit
{
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

static VKAPI_ATTR VkBool32 VKAPI_CALL defaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT p_Severity,
                                                           VkDebugUtilsMessageTypeFlagsEXT p_MessageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT *p_CallbackData,
                                                           void *)
{
    TKIT_ERROR("<{}: {}> {}", toString(p_Severity), toString(p_MessageType), p_CallbackData->pMessage);
    return VK_FALSE;
}

static bool contains(const DynamicArray<const char *> &p_Extensions, const char *p_Extension) noexcept
{
    return std::find(p_Extensions.begin(), p_Extensions.end(), p_Extension) != p_Extensions.end();
}

Result<Instance> Instance::Specs::Create() const noexcept
{
    const auto checkApiVersion = [](const u32 p_Version, const bool p_IsRequested) -> Result<u32> {
        if (p_Version < VK_API_VERSION_1_1)
            return Result<u32>::Ok(p_Version); // You just cant check versions at this point

        const auto enumerateVersion =
            System::GetVulkanFunction<PFN_vkEnumerateInstanceVersion>("vkEnumerateInstanceVersion");
        if (!enumerateVersion)
            return Result<u32>::Error(
                VKIT_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT, "Failed to get the vkEnumerateInstanceVersion function"));

        u32 version;
        const VkResult result = enumerateVersion(&version);
        if (result != VK_SUCCESS)
            return Result<u32>::Error(VKIT_ERROR(result, "Failed to get the vulkan instance version"));

        if (version < p_Version)
            return Result<u32>::Error(VKIT_ERROR(
                VK_ERROR_INCOMPATIBLE_DRIVER,
                "The vulkan instance version {} is not supported, the required version is {}", version, p_Version));

        return Result<u32>::Ok(p_IsRequested ? p_Version : version);
    };

    TKIT_ASSERT(m_RequestedApiVersion >= m_RequiredApiVersion,
                "The requested api version must be greater than or equal to the required api version");

    Result<u32> vresult = checkApiVersion(m_RequestedApiVersion, true);
    if (!vresult)
    {
        vresult = checkApiVersion(m_RequiredApiVersion, false);
        if (!vresult)
            return Result<Instance>::Error(vresult.GetError());
    }
    const u32 apiVersion = vresult.GetValue();

    for (const char *extension : m_RequiredExtensions)
        if (!System::IsExtensionSupported(extension))
            return Result<Instance>::Error(
                VKIT_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT, "The extension {} is not suported", extension));

    for (const char *layer : m_RequiredLayers)
        if (!System::IsLayerSupported(layer))
            return Result<Instance>::Error(
                VKIT_ERROR(VK_ERROR_LAYER_NOT_PRESENT, "The layer {} is not suported", layer));

    DynamicArray<const char *> extensions{m_RequiredExtensions.begin(), m_RequiredExtensions.end()};
    for (const char *extension : m_RequestedExtensions)
    {
        const bool supported = System::IsExtensionSupported(extension);
        TKIT_LOG_WARNING_IF(!supported, "The extension {} is not suported", extension);
        if (supported)
            extensions.push_back(extension);
    }

    DynamicArray<const char *> layers{m_RequiredLayers.begin(), m_RequiredLayers.end()};
    for (const char *layer : m_RequestedLayers)
    {
        const bool supported = System::IsLayerSupported(layer);
        TKIT_LOG_WARNING_IF(!supported, "The layer {} is not suported", layer);
        if (supported)
            layers.push_back(layer);
    }

    bool validationLayers = false;
    if (m_RequestValidationLayers)
    {
        validationLayers = System::IsLayerSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        if (!validationLayers && m_RequireValidationLayers)
            return Result<Instance>::Error(
                VKIT_ERROR(VK_ERROR_LAYER_NOT_PRESENT, "Validation layers are not suported"));
        if (!contains(layers, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
            layers.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    const bool properties2Support =
        apiVersion < VK_API_VERSION_1_1 &&
        System::IsExtensionSupported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    if (properties2Support && !contains(extensions, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#ifdef VK_KHR_portability_enumeration
    const bool portabilitySupport = System::IsExtensionSupported(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    if (portabilitySupport && !contains(extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    if (!m_Headless)
    {
        const auto checkWindowingSupport = [&extensions](const char *p_Extension) -> bool {
            if (!System::IsExtensionSupported(p_Extension))
                return false;
            if (!contains(extensions, p_Extension))
                extensions.push_back(p_Extension);
            return true;
        };

        const auto generateError = [](const char *p_Extension) -> Result<Instance> {
            return Result<Instance>::Error(
                VKIT_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT,
                           "The extension {}, required for windowing capabilities, is not suported", p_Extension));
        };

        if (!checkWindowingSupport(VK_KHR_SURFACE_EXTENSION_NAME))
            return generateError(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef TKIT_OS_WINDOWS
        if (!checkWindowingSupport("VK_KHR_win32_surface"))
            return generateError("VK_KHR_win32_surface");

#elif defined(TKIT_OS_APPLE)
        if (!checkWindowingSupport("VK_EXT_metal_surface"))
            return generateError("VK_EXT_metal_surface");

#elif defined(TKIT_OS_ANDROID)
        if (!checkWindowingSupport("VK_KHR_android_surface"))
            return generateError("VK_KHR_android_surface");

#elif defined(_DIRECT2DISPLAY)
        if (!checkWindowingSupport("VK_KHR_display"))
            return generateError("VK_KHR_display");

#elif defined(TKIT_OS_LINUX)
        if (!checkWindowingSupport("VK_KHR_xcb_surface") && !checkWindowingSupport("VK_KHR_xlib_surface") &&
            !checkWindowingSupport("VK_KHR_wayland_surface"))
            return generateError("VK_KHR_[xcb|xlib|wayland]_surface");
#endif
    }

    VkDebugUtilsMessengerCreateInfoEXT msgInfo{};
    if (validationLayers)
    {
        msgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        msgInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        msgInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        msgInfo.pfnUserCallback = m_DebugCallback ? m_DebugCallback : defaultDebugCallback;
        msgInfo.pUserData = m_DebugMessengerUserData;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = m_ApplicationName;
    appInfo.applicationVersion = m_ApplicationVersion;
    appInfo.pEngineName = m_EngineName;
    appInfo.engineVersion = m_EngineVersion;
    appInfo.apiVersion = apiVersion;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    instanceInfo.enabledLayerCount = static_cast<u32>(layers.size());
    instanceInfo.ppEnabledLayerNames = layers.data();
    instanceInfo.pNext = validationLayers ? &msgInfo : nullptr;
#ifdef VK_KHR_portability_enumeration
    if (portabilitySupport)
        instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkInstance vkinstance;
    const auto createInstance = System::GetVulkanFunction<PFN_vkCreateInstance>("vkCreateInstance");
    const auto destroyInstance = System::GetVulkanFunction<PFN_vkDestroyInstance>("vkDestroyInstance");

    if (!createInstance)
        return Result<Instance>::Error(
            VKIT_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT, "Failed to get the vkCreateInstance function"));
    if (!destroyInstance)
        return Result<Instance>::Error(
            VKIT_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT, "Failed to get the vkDestroyInstance function"));

    VkResult result = createInstance(&instanceInfo, m_AllocationCallbacks, &vkinstance);
    if (result != VK_SUCCESS)
        return Result<Instance>::Error(VKIT_ERROR(result, "Failed to create the vulkan instance"));

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    if (validationLayers)
    {
        const auto createDebugMessenger =
            System::GetVulkanFunction<PFN_vkCreateDebugUtilsMessengerEXT>("vkCreateDebugUtilsMessengerEXT");
        if (!createDebugMessenger)
            return Result<Instance>::Error(VKIT_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT,
                                                      "Failed to get the vkCreateDebugUtilsMessengerEXT function"));

        result = createDebugMessenger(vkinstance, &msgInfo, nullptr, &debugMessenger);
        if (result != VK_SUCCESS)
        {
            destroyInstance(vkinstance, m_AllocationCallbacks);
            return Result<Instance>::Error(VKIT_ERROR(result, "Failed to create the debug messenger"));
        }
    }

    Instance::Info info{};
    info.ApplicationName = m_ApplicationName;
    info.ApplicationVersion = m_ApplicationVersion;
    info.EngineName = m_EngineName;
    info.EngineVersion = m_EngineVersion;
    info.Headless = m_Headless;
    info.ApiVersion = apiVersion;
    info.Extensions = extensions;
    info.Layers = layers;
    info.HasValidationLayers = validationLayers;
    info.Properties2Extension = properties2Support;
    info.AllocationCallbacks = m_AllocationCallbacks;
    info.DebugMessenger = debugMessenger;

    TKIT_ASSERT((validationLayers && debugMessenger) || (!validationLayers && !debugMessenger),
                "The debug messenger must be available if validation layers are enabled");

    const Instance instance{vkinstance, info};
    return Result<Instance>::Ok(instance);
}

Instance::Instance(VkInstance p_Instance, const Info &p_Info) noexcept : m_Instance(p_Instance), m_Info(p_Info)
{
}

void Instance::Destroy(const Instance &p_Instance) noexcept
{
    // Should be already available if user managed to create instance
    const auto destroyInstance = System::GetVulkanFunction<PFN_vkDestroyInstance>("vkDestroyInstance");
    if (p_Instance.m_Info.HasValidationLayers)
    {
        const auto destroyDebugMessenger =
            System::GetVulkanFunction<PFN_vkDestroyDebugUtilsMessengerEXT>("vkDestroyDebugUtilsMessengerEXT");
        TKIT_ASSERT(destroyDebugMessenger, "Failed to get the vkDestroyDebugUtilsMessengerEXT function");
        destroyDebugMessenger(p_Instance.m_Instance, p_Instance.m_Info.DebugMessenger,
                              p_Instance.m_Info.AllocationCallbacks);
    }
    destroyInstance(p_Instance.m_Instance, p_Instance.m_Info.AllocationCallbacks);
}

VkInstance Instance::GetInstance() const noexcept
{
    return m_Instance;
}
const Instance::Info &Instance::GetInfo() const noexcept
{
    return m_Info;
}

} // namespace VKit