#include "vkit/core/pch.hpp"
#include "vkit/core/core.hpp"
#include "vkit/vulkan/instance.hpp"
#include "tkit/utils/debug.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/span.hpp"

namespace VKit
{
static const char *toString(const VkDebugUtilsMessageTypeFlagsEXT messageType)
{
    if (messageType == 7)
        return "General | Validation | Performance";
    if (messageType == 6)
        return "Validation | Performance";
    if (messageType == 5)
        return "General | Performance";
    if (messageType == 4)
        return "Performance";
    if (messageType == 3)
        return "General | Validation";
    if (messageType == 2)
        return "Validation";
    if (messageType == 1)
        return "General";
    return "Unknown";
}

static VKAPI_ATTR VkBool32 VKAPI_CALL defaultDebugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                           const VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
                                                           void *)
{
    switch (severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        TKIT_LOG_DEBUG("[VULKIT][{}] {}", toString(messageType), callbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        TKIT_FATAL("[VULKIT][{}] {}", toString(messageType), callbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        TKIT_LOG_WARNING("[VULKIT][{}] {}", toString(messageType), callbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        TKIT_LOG_INFO("[VULKIT][{}] {}", toString(messageType), callbackData->pMessage);
        break;
    default:
        TKIT_LOG_INFO("[VULKIT][{}] {}", toString(messageType), callbackData->pMessage);
        break;
    }
    return VK_FALSE;
}

static bool contains(const TKit::Span<const char *const> extensions, const char *extension)
{
    return std::find(extensions.begin(), extensions.end(), extension) != extensions.end();
}

Instance::Builder::Builder()
{
    m_RequiredExtensions.Reserve(Core::GetExtensionCount());
    m_RequestedExtensions.Reserve(Core::GetExtensionCount());

    m_RequiredLayers.Reserve(Core::GetLayerCount());
    m_RequestedLayers.Reserve(Core::GetLayerCount());
}

Result<Instance> Instance::Builder::Build() const
{
    const auto checkApiVersion = [](const u32 pversion, const bool isRequested) -> Result<u32> {
#ifdef VKIT_API_VERSION_1_1
        u32 version;
        VKIT_RETURN_IF_FAILED(Vulkan::EnumerateInstanceVersion(&version), Result<u32>);
#else
        const u32 version = VKIT_MAKE_VERSION(0, 1, 0, 0);
#endif
        if (version < pversion)
            return Result<u32>::Error(Error_VersionMismatch,
                                      TKit::Format("[VULKIT][INSTANCE] The vulkan instance version {}.{}.{} found is "
                                                   "not supported. The required version is {}.{}.{}",
                                                   VKIT_EXPAND_VERSION(version), VKIT_EXPAND_VERSION(version)));

        return isRequested ? pversion : version;
    };

    TKIT_ASSERT(
        m_RequestedApiVersion >= m_RequiredApiVersion,
        "[VULKIT][INSTANCE] The requested api version ({}.{}.{}) must be greater than or equal to the required api "
        "version ({}.{}.{})",
        VKIT_EXPAND_VERSION(m_RequestedApiVersion), VKIT_EXPAND_VERSION(m_RequiredApiVersion));

    Result<u32> vresult = checkApiVersion(m_RequestedApiVersion, true);
    if (!vresult)
    {
        TKIT_LOG_WARNING("[VULKIT][INSTANCE] The requested version {}.{}.{} is not available. Trying {}.{}.{}",
                         VKIT_EXPAND_VERSION(m_RequestedApiVersion), VKIT_EXPAND_VERSION(m_RequiredApiVersion));

        vresult = checkApiVersion(m_RequiredApiVersion, false);
        TKIT_RETURN_ON_ERROR(vresult);
    }
    const u32 apiVersion = vresult.GetValue();

    for (const char *extension : m_RequiredExtensions)
        if (!Core::IsExtensionSupported(extension))
            return Result<Instance>::Error(
                Error_MissingExtension,
                TKit::Format("[VULKIT][INSTANCE] The required extension '{}' is not suported", extension));

    for (const char *layer : m_RequiredLayers)
        if (!Core::IsLayerSupported(layer))
            return Result<Instance>::Error(
                Error_MissingLayer, TKit::Format("[VULKIT][INSTANCE] The required layer '{}' is not suported", layer));

    TKit::StackArray<const char *> extensions;
    extensions.Reserve(m_RequiredExtensions.GetCapacity());
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

    TKit::StackArray<const char *> layers;
    layers.Reserve(m_RequiredLayers.GetCapacity());
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
            return Result<Instance>::Error(
                Error_MissingLayer,
                "[VULKIT][INSTANCE] Validation layers (along with the debug utils extension) are not suported");

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
        const auto checkWindowingSupport = [&extensions](const char *extension) -> bool {
            if (!Core::IsExtensionSupported(extension))
                return false;
            if (!contains(extensions, extension))
                extensions.Append(extension);
            return true;
        };

        const auto generateError = [](const char *extension) -> Result<Instance> {
            return Result<Instance>::Error(
                Error_MissingExtension,
                TKit::Format(
                    "[VULKIT][INSTANCE] The extension '{}', required for windowing capabilities, is not suported",
                    extension));
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

    VkInstance vkinstance;
    VKIT_RETURN_IF_FAILED(Vulkan::CreateInstance(&instanceInfo, m_AllocationCallbacks, &vkinstance), Result<Instance>);

    TKit::TierAllocator *alloc = TKit::GetTier();
    Vulkan::InstanceTable *table = alloc->Create<Vulkan::InstanceTable>(Vulkan::InstanceTable::Create(vkinstance));

#ifdef VK_EXT_debug_utils
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
#endif

    const auto cleanup = [&] {
#ifdef VK_EXT_debug_utils
        table->DestroyDebugUtilsMessengerEXT(vkinstance, debugMessenger, m_AllocationCallbacks);
#endif
        table->DestroyInstance(vkinstance, m_AllocationCallbacks);
        alloc->Destroy(table);
    };

    if (validationLayers)
    {
#ifdef VK_EXT_debug_utils
        VKIT_RETURN_IF_FAILED(
            table->CreateDebugUtilsMessengerEXT(vkinstance, &msgInfo, m_AllocationCallbacks, &debugMessenger),
            Result<Instance>, cleanup());
#else
        cleanup();
        return Result<Instance>::Error(
            Error_MissingLayer,
            "[VULKIT][INSTANCE] Validation layers (along with the debug utils extension) are not suported");
#endif
    }

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

    if (!((validationLayers && debugMessenger) || (!validationLayers && !debugMessenger)))
    {
        cleanup();
        return Result<Instance>::Error(
            Error_MissingLayer,
            "[VULKIT][INSTANCE] The debug messenger must be available if validation layers are enabled");
    }

    return Result<Instance>::Ok(vkinstance, info);
}

bool Instance::IsExtensionEnabled(const char *extension) const
{
    return contains(m_Info.EnabledExtensions, extension);
}
bool Instance::IsLayerEnabled(const char *layer) const
{
    return contains(m_Info.EnabledLayers, layer);
}

void Instance::Destroy()
{
    if (!m_Instance)
        return;

    if ((m_Info.Flags & InstanceFlag_HasValidationLayers) && m_Info.DebugMessenger)
    {
        m_Info.Table->DestroyDebugUtilsMessengerEXT(m_Instance, m_Info.DebugMessenger, m_Info.AllocationCallbacks);
        m_Info.DebugMessenger = VK_NULL_HANDLE;
    }

    m_Info.Table->DestroyInstance(m_Instance, m_Info.AllocationCallbacks);
    TKit::GetTier()->Destroy(m_Info.Table);
    m_Instance = VK_NULL_HANDLE;
}

Instance::Proxy Instance::CreateProxy() const
{
    Proxy proxy;
    proxy.Instance = m_Instance;
    proxy.AllocationCallbacks = m_Info.AllocationCallbacks;
    proxy.Table = m_Info.Table;
    return proxy;
}

Instance::Builder &Instance::Builder::SetApplicationName(const char *name)
{
    m_ApplicationName = name;
    return *this;
}
Instance::Builder &Instance::Builder::SetEngineName(const char *name)
{
    m_EngineName = name;
    return *this;
}
Instance::Builder &Instance::Builder::SetApplicationVersion(u32 version)
{
    m_ApplicationVersion = version;
    return *this;
}
Instance::Builder &Instance::Builder::SetEngineVersion(u32 version)
{
    m_EngineVersion = version;
    return *this;
}
Instance::Builder &Instance::Builder::SetApplicationVersion(u32 major, u32 minor, u32 patch)
{
    return SetApplicationVersion(VKIT_MAKE_VERSION(0, major, minor, patch));
}
Instance::Builder &Instance::Builder::SetEngineVersion(u32 major, u32 minor, u32 patch)
{
    return SetEngineVersion(VKIT_MAKE_VERSION(0, major, minor, patch));
}
Instance::Builder &Instance::Builder::RequireApiVersion(u32 version)
{
    m_RequiredApiVersion = version;
    if (m_RequestedApiVersion < m_RequiredApiVersion)
        m_RequestedApiVersion = m_RequiredApiVersion;
    return *this;
}
Instance::Builder &Instance::Builder::RequireApiVersion(u32 major, u32 minor, u32 patch)
{
    return RequireApiVersion(VKIT_MAKE_VERSION(0, major, minor, patch));
}
Instance::Builder &Instance::Builder::RequestApiVersion(u32 version)
{
    m_RequestedApiVersion = version;
    if (m_RequestedApiVersion < m_RequiredApiVersion)
        m_RequiredApiVersion = m_RequestedApiVersion;
    return *this;
}
Instance::Builder &Instance::Builder::RequestApiVersion(u32 major, u32 minor, u32 patch)
{
    return RequestApiVersion(VKIT_MAKE_VERSION(0, major, minor, patch));
}
Instance::Builder &Instance::Builder::RequireExtension(const char *extension)
{
    m_RequiredExtensions.Append(extension);
    return *this;
}
Instance::Builder &Instance::Builder::RequestExtension(const char *extension)
{
    m_RequestedExtensions.Append(extension);
    return *this;
}
Instance::Builder &Instance::Builder::RequireLayer(const char *layer)
{
    m_RequiredLayers.Append(layer);
    return *this;
}
Instance::Builder &Instance::Builder::RequestLayer(const char *layer)
{
    m_RequestedLayers.Append(layer);
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
Instance::Builder &Instance::Builder::SetDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT callback)
{
    m_DebugCallback = callback;
    return *this;
}
Instance::Builder &Instance::Builder::SetHeadless(bool headless)
{
    m_Headless = headless;
    return *this;
}
Instance::Builder &Instance::Builder::SetDebugMessengerUserData(void *data)
{
    m_DebugMessengerUserData = data;
    return *this;
}
Instance::Builder &Instance::Builder::SetAllocationCallbacks(const VkAllocationCallbacks *allocationCallbacks)
{
    m_AllocationCallbacks = allocationCallbacks;
    return *this;
}

} // namespace VKit
