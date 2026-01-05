#include "vkit/core/pch.hpp"
#include "vkit/core/core.hpp"
#include "vkit/vulkan/instance.hpp"
#include "tkit/utils/debug.hpp"

#define EXPAND_VERSION(p_Version)                                                                                      \
    VKIT_API_VERSION_MAJOR(p_Version), VKIT_API_VERSION_MINOR(p_Version), VKIT_API_VERSION_PATCH(p_Version)

namespace VKit
{
static const char *toString(const VkDebugUtilsMessageTypeFlagsEXT p_MessageType)
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

static VKAPI_ATTR VkBool32 VKAPI_CALL defaultDebugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT p_Severity,
                                                           const VkDebugUtilsMessageTypeFlagsEXT p_MessageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT *p_CallbackData,
                                                           void *)
{
    switch (p_Severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        TKIT_LOG_DEBUG("[VULKIT][{}] {}", toString(p_MessageType), p_CallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        TKIT_FATAL("[VULKIT][{}] {}", toString(p_MessageType), p_CallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        TKIT_LOG_WARNING("[VULKIT][{}] {}", toString(p_MessageType), p_CallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        TKIT_LOG_INFO("[VULKIT][{}] {}", toString(p_MessageType), p_CallbackData->pMessage);
        break;
    default:
        TKIT_LOG_INFO("[VULKIT][{}] {}", toString(p_MessageType), p_CallbackData->pMessage);
        break;
    }
    return VK_FALSE;
}

static bool contains(const TKit::Span<const char *const> p_Extensions, const char *p_Extension)
{
    return std::find(p_Extensions.begin(), p_Extensions.end(), p_Extension) != p_Extensions.end();
}

Result<Instance> Instance::Builder::Build() const
{
    const auto checkApiVersion = [](const u32 p_Version, const bool p_IsRequested) -> Result<u32> {
#ifdef VKIT_API_VERSION_1_1
        VKIT_CHECK_GLOBAL_FUNCTION_OR_RETURN(vkEnumerateInstanceVersion, Result<u32>);

        u32 version;
        const VkResult result = Vulkan::EnumerateInstanceVersion(&version);
        if (result != VK_SUCCESS)
            return Result<u32>::Error(result);
#else
        const u32 version = VKIT_MAKE_VERSION(0, 1, 0, 0);
#endif
        if (version < p_Version)
            return Result<u32>::Error(
                Error_VersionMismatch,
                TKit::Format(
                    "The vulkan instance version {}.{}.{} found is not supported. The required version is {}.{}.{}",
                    EXPAND_VERSION(version), EXPAND_VERSION(p_Version)));

        return p_IsRequested ? p_Version : version;
    };

    TKIT_ASSERT(
        m_RequestedApiVersion >= m_RequiredApiVersion,
        "[VULKIT][INSTANCE] The requested api version ({}.{}.{}) must be greater than or equal to the required api "
        "version ({}.{}.{})",
        EXPAND_VERSION(m_RequestedApiVersion), EXPAND_VERSION(m_RequiredApiVersion));

    Result<u32> vresult = checkApiVersion(m_RequestedApiVersion, true);
    if (!vresult)
    {
        TKIT_LOG_WARNING("[VULKIT][INSTANCE] The requested version {}.{}.{} is not available. Trying {}.{}.{}",
                         EXPAND_VERSION(m_RequestedApiVersion), EXPAND_VERSION(m_RequiredApiVersion));

        vresult = checkApiVersion(m_RequiredApiVersion, false);
        TKIT_RETURN_ON_ERROR(vresult);
    }
    const u32 apiVersion = vresult.GetValue();

    for (const char *extension : m_RequiredExtensions)
        if (!Core::IsExtensionSupported(extension))
            return Result<Instance>::Error(Error_MissingExtension,
                                           TKit::Format("The required extension '{}' is not suported", extension));

    for (const char *layer : m_RequiredLayers)
        if (!Core::IsLayerSupported(layer))
            return Result<Instance>::Error(Error_MissingLayer,
                                           TKit::Format("The required layer '{}' is not suported", layer));

    TKit::Array64<const char *> extensions;
    for (const char *extension : m_RequiredExtensions)
        if (!contains(extensions, extension))
            extensions.Append(extension);

    for (const char *extension : m_RequestedExtensions)
    {
        const bool supported = Core::IsExtensionSupported(extension);
        TKIT_LOG_WARNING_IF(!supported, "[VULKIT][INSTANCE] The requested extension '{}' is not suported", extension);
        if (supported && !contains(extensions, extension))
            extensions.Append(extension);
    }

    TKit::Array16<const char *> layers;
    for (const char *layer : m_RequiredLayers)
        if (!contains(layers, layer))
            layers.Append(layer);

    for (const char *layer : m_RequestedLayers)
    {
        const bool supported = Core::IsLayerSupported(layer);
        TKIT_LOG_WARNING_IF(!supported, "[VULKIT][INSTANCE] The requested layer '{}' is not suported", layer);
        if (supported && !contains(layers, layer))
            layers.Append(layer);
    }

    bool validationLayers = false;
    if (m_RequestValidationLayers)
    {
        validationLayers =
            Core::IsExtensionSupported("VK_EXT_debug_utils") && Core::IsLayerSupported("VK_LAYER_KHRONOS_validation");

        if (!validationLayers && m_RequireValidationLayers)
            return Result<Instance>::Error(Error_MissingLayer,
                                           "Validation layers (along with the debug utils extension) are not suported");

        TKIT_LOG_WARNING_IF(
            !validationLayers && m_RequestValidationLayers,
            "[VULKIT][INSTANCE] Validation layers (along with the debug utils extension) are not suported");

        if (validationLayers && !contains(extensions, "VK_EXT_debug_utils"))
            extensions.Append("VK_EXT_debug_utils");

        if (validationLayers && !contains(layers, "VK_LAYER_KHRONOS_validation"))
            layers.Append("VK_LAYER_KHRONOS_validation");
    }

    const bool properties2Support = apiVersion < VKIT_MAKE_VERSION(0, 1, 1, 0) &&
                                    Core::IsExtensionSupported("VK_KHR_get_physical_device_properties2");

    if (properties2Support && !contains(extensions, "VK_KHR_get_physical_device_properties2"))
        extensions.Append("VK_KHR_get_physical_device_properties2");

#ifdef VK_KHR_portability_enumeration
    const bool portabilitySupport = Core::IsExtensionSupported("VK_KHR_portability_enumeration");
    if (portabilitySupport && !contains(extensions, "VK_KHR_portability_enumeration"))
        extensions.Append("VK_KHR_portability_enumeration");
#endif

    if (!m_Headless)
    {
        const auto checkWindowingSupport = [&extensions](const char *p_Extension) -> bool {
            if (!Core::IsExtensionSupported(p_Extension))
                return false;
            if (!contains(extensions, p_Extension))
                extensions.Append(p_Extension);
            return true;
        };

        const auto generateError = [](const char *p_Extension) -> Result<Instance> {
            return Result<Instance>::Error(
                Error_MissingExtension,
                TKit::Format("The extension '{}', required for windowing capabilities, is not suported", p_Extension));
        };

        if (!checkWindowingSupport("VK_KHR_surface"))
            return generateError("VK_KHR_surface");

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
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
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
    instanceInfo.enabledExtensionCount = extensions.GetSize();
    instanceInfo.ppEnabledExtensionNames = extensions.GetData();
    instanceInfo.enabledLayerCount = layers.GetSize();
    instanceInfo.ppEnabledLayerNames = layers.GetData();
    instanceInfo.pNext = validationLayers ? &msgInfo : nullptr;
#ifdef VK_KHR_portability_enumeration
    if (portabilitySupport)
        instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VKIT_CHECK_GLOBAL_FUNCTION_OR_RETURN(vkCreateInstance, Result<Instance>);

    VkInstance vkinstance;
    VkResult result = Vulkan::CreateInstance(&instanceInfo, m_AllocationCallbacks, &vkinstance);
    if (result != VK_SUCCESS)
        return Result<Instance>::Error(result);

    const Vulkan::InstanceTable table = Vulkan::InstanceTable::Create(vkinstance);

    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN((&table), vkDestroyInstance, Result<Instance>);

#ifdef VK_EXT_debug_utils
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    if (validationLayers)
    {
        if (!table.vkCreateDebugUtilsMessengerEXT || !table.vkDestroyDebugUtilsMessengerEXT)
        {
            table.DestroyInstance(vkinstance, m_AllocationCallbacks);
            return Result<Instance>::Error(Error_VulkanFunctionNotLoaded, "Failed to load Vulkan functions: "
                                                                              "vkCreate/DestroyDebugUtilsMessengerEXT");
        }
        result = table.CreateDebugUtilsMessengerEXT(vkinstance, &msgInfo, nullptr, &debugMessenger);
        if (result != VK_SUCCESS)
        {
            table.DestroyInstance(vkinstance, m_AllocationCallbacks);
            return Result<Instance>::Error(result);
        }
    }
#else
    if (validationLayers)
    {
        table.DestroyInstance(vkinstance, m_AllocationCallbacks);
        return Result<Instance>::Error(Error_MissingLayer,
                                       "Validation layers (along with the debug utils extension) are not suported");
    }
#endif

    Instance::Info info{};
    info.ApplicationName = m_ApplicationName;
    info.ApplicationVersion = m_ApplicationVersion;
    info.EngineName = m_EngineName;
    info.EngineVersion = m_EngineVersion;
    info.ApiVersion = apiVersion;
    info.EnabledExtensions = extensions;
    info.EnabledLayers = layers;
    info.AllocationCallbacks = m_AllocationCallbacks;
    info.DebugMessenger = debugMessenger;
    info.Table = table;
    if (m_Headless)
        info.Flags |= InstanceFlag_Headless;
    if (validationLayers)
        info.Flags |= InstanceFlag_HasValidationLayers;
    if (properties2Support)
        info.Flags |= InstanceFlag_Properties2Extension;

    TKIT_ASSERT((validationLayers && debugMessenger) || (!validationLayers && !debugMessenger),
                "[VULKIT][INSTANCE] The debug messenger must be available if validation layers are enabled");

    return Result<Instance>::Ok(vkinstance, info);
}

bool Instance::IsExtensionEnabled(const char *p_Extension) const
{
    return contains(m_Info.EnabledExtensions, p_Extension);
}
bool Instance::IsLayerEnabled(const char *p_Layer) const
{
    return contains(m_Info.EnabledLayers, p_Layer);
}

void Instance::Destroy()
{
    if (!m_Instance)
        return;

    if ((m_Info.Flags & InstanceFlag_HasValidationLayers) && m_Info.DebugMessenger)
    {
        m_Info.Table.DestroyDebugUtilsMessengerEXT(m_Instance, m_Info.DebugMessenger, m_Info.AllocationCallbacks);
        m_Info.DebugMessenger = VK_NULL_HANDLE;
    }

    m_Info.Table.DestroyInstance(m_Instance, m_Info.AllocationCallbacks);
    m_Instance = VK_NULL_HANDLE;
}

Instance::Proxy Instance::CreateProxy() const
{
    Proxy proxy;
    proxy.Instance = m_Instance;
    proxy.AllocationCallbacks = m_Info.AllocationCallbacks;
    proxy.Table = &m_Info.Table;
    return proxy;
}

Instance::Builder &Instance::Builder::SetApplicationName(const char *p_Name)
{
    m_ApplicationName = p_Name;
    return *this;
}
Instance::Builder &Instance::Builder::SetEngineName(const char *p_Name)
{
    m_EngineName = p_Name;
    return *this;
}
Instance::Builder &Instance::Builder::SetApplicationVersion(u32 p_Version)
{
    m_ApplicationVersion = p_Version;
    return *this;
}
Instance::Builder &Instance::Builder::SetEngineVersion(u32 p_Version)
{
    m_EngineVersion = p_Version;
    return *this;
}
Instance::Builder &Instance::Builder::SetApplicationVersion(u32 p_Major, u32 p_Minor, u32 p_Patch)
{
    return SetApplicationVersion(VKIT_MAKE_VERSION(0, p_Major, p_Minor, p_Patch));
}
Instance::Builder &Instance::Builder::SetEngineVersion(u32 p_Major, u32 p_Minor, u32 p_Patch)
{
    return SetEngineVersion(VKIT_MAKE_VERSION(0, p_Major, p_Minor, p_Patch));
}
Instance::Builder &Instance::Builder::RequireApiVersion(u32 p_Version)
{
    m_RequiredApiVersion = p_Version;
    if (m_RequestedApiVersion < m_RequiredApiVersion)
        m_RequestedApiVersion = m_RequiredApiVersion;
    return *this;
}
Instance::Builder &Instance::Builder::RequireApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch)
{
    return RequireApiVersion(VKIT_MAKE_VERSION(0, p_Major, p_Minor, p_Patch));
}
Instance::Builder &Instance::Builder::RequestApiVersion(u32 p_Version)
{
    m_RequestedApiVersion = p_Version;
    if (m_RequestedApiVersion < m_RequiredApiVersion)
        m_RequiredApiVersion = m_RequestedApiVersion;
    return *this;
}
Instance::Builder &Instance::Builder::RequestApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch)
{
    return RequestApiVersion(VKIT_MAKE_VERSION(0, p_Major, p_Minor, p_Patch));
}
Instance::Builder &Instance::Builder::RequireExtension(const char *p_Extension)
{
    m_RequiredExtensions.Append(p_Extension);
    return *this;
}
Instance::Builder &Instance::Builder::RequireExtensions(TKit::Span<const char *const> p_Extensions)
{
    m_RequiredExtensions.Insert(m_RequiredExtensions.end(), p_Extensions.begin(), p_Extensions.end());
    return *this;
}
Instance::Builder &Instance::Builder::RequestExtension(const char *p_Extension)
{
    m_RequestedExtensions.Append(p_Extension);
    return *this;
}
Instance::Builder &Instance::Builder::RequestExtensions(TKit::Span<const char *const> p_Extensions)
{
    m_RequestedExtensions.Insert(m_RequestedExtensions.end(), p_Extensions.begin(), p_Extensions.end());
    return *this;
}
Instance::Builder &Instance::Builder::RequireLayer(const char *p_Layer)
{
    m_RequiredLayers.Append(p_Layer);
    return *this;
}
Instance::Builder &Instance::Builder::RequireLayers(TKit::Span<const char *const> p_Layers)
{
    m_RequiredLayers.Insert(m_RequiredLayers.end(), p_Layers.begin(), p_Layers.end());
    return *this;
}
Instance::Builder &Instance::Builder::RequestLayer(const char *p_Layer)
{
    m_RequestedLayers.Append(p_Layer);
    return *this;
}
Instance::Builder &Instance::Builder::RequestLayers(TKit::Span<const char *const> p_Layers)
{
    m_RequestedLayers.Insert(m_RequestedLayers.end(), p_Layers.begin(), p_Layers.end());
    return *this;
}
Instance::Builder &Instance::Builder::RequireValidationLayers()
{
    m_RequireValidationLayers = true;
    m_RequestValidationLayers = true;
    return *this;
}
Instance::Builder &Instance::Builder::RequestValidationLayers()
{
    m_RequestValidationLayers = true;
    return *this;
}
Instance::Builder &Instance::Builder::SetDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT p_Callback)
{
    m_DebugCallback = p_Callback;
    return *this;
}
Instance::Builder &Instance::Builder::SetHeadless(bool p_Headless)
{
    m_Headless = p_Headless;
    return *this;
}
Instance::Builder &Instance::Builder::SetDebugMessengerUserData(void *p_Data)
{
    m_DebugMessengerUserData = p_Data;
    return *this;
}
Instance::Builder &Instance::Builder::SetAllocationCallbacks(const VkAllocationCallbacks *p_AllocationCallbacks)
{
    m_AllocationCallbacks = p_AllocationCallbacks;
    return *this;
}

} // namespace VKit
