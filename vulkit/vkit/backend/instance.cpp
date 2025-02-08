#include "vkit/core/pch.hpp"
#include "vkit/backend/instance.hpp"
#include "vkit/core/alias.hpp"
#include "tkit/utils/logging.hpp"

namespace VKit
{
#ifdef TKIT_ENABLE_ASSERTS
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
#endif

static bool contains(const TKit::Span<const char *const> p_Extensions, const char *p_Extension) noexcept
{
    return std::find(p_Extensions.begin(), p_Extensions.end(), p_Extension) != p_Extensions.end();
}

FormattedResult<Instance> Instance::Builder::Build() const noexcept
{
    const auto checkApiVersion = [](const u32 p_Version, const bool p_IsRequested) -> FormattedResult<u32> {
        if (p_Version < VKIT_MAKE_VERSION(0, 1, 1, 0))
            return FormattedResult<u32>::Ok(p_Version); // You just cant check versions at this point

        u32 version;
        const VkResult result = vkEnumerateInstanceVersion(&version);
        if (result != VK_SUCCESS)
            return FormattedResult<u32>::Error(result, "Failed to get the vulkan instance version");

        if (version < p_Version)
            return FormattedResult<u32>::Error(VKIT_FORMAT_ERROR(
                VK_ERROR_INCOMPATIBLE_DRIVER,
                "The vulkan instance version {} is not supported, the required version is {}", version, p_Version));

        return FormattedResult<u32>::Ok(p_IsRequested ? p_Version : version);
    };

    TKIT_ASSERT(m_RequestedApiVersion >= m_RequiredApiVersion,
                "[VULKIT] The requested api version must be greater than or equal to the required api version");

    FormattedResult<u32> vresult = checkApiVersion(m_RequestedApiVersion, true);
    if (!vresult)
    {
        vresult = checkApiVersion(m_RequiredApiVersion, false);
        if (!vresult)
            return FormattedResult<Instance>::Error(vresult.GetError());
    }
    const u32 apiVersion = vresult.GetValue();

    for (const char *extension : m_RequiredExtensions)
        if (!System::IsExtensionSupported(extension))
            return FormattedResult<Instance>::Error(
                VKIT_FORMAT_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT, "The extension {} is not suported", extension));

    for (const char *layer : m_RequiredLayers)
        if (!System::IsLayerSupported(layer))
            return FormattedResult<Instance>::Error(
                VKIT_FORMAT_ERROR(VK_ERROR_LAYER_NOT_PRESENT, "The layer {} is not suported", layer));

    TKit::StaticArray64<const char *> extensions;
    for (const char *extension : m_RequiredExtensions)
        if (!contains(extensions, extension))
            extensions.push_back(extension);

    for (const char *extension : m_RequestedExtensions)
    {
        const bool supported = System::IsExtensionSupported(extension);
        TKIT_LOG_WARNING_IF(!supported, "[VULKIT] The extension {} is not suported", extension);
        if (supported && !contains(extensions, extension))
            extensions.push_back(extension);
    }

    TKit::StaticArray16<const char *> layers;
    for (const char *layer : m_RequiredLayers)
        if (!contains(layers, layer))
            layers.push_back(layer);

    for (const char *layer : m_RequestedLayers)
    {
        const bool supported = System::IsLayerSupported(layer);
        TKIT_LOG_WARNING_IF(!supported, "[VULKIT] The layer {} is not suported", layer);
        if (supported && !contains(layers, layer))
            layers.push_back(layer);
    }

    bool validationLayers = false;
    if (m_RequestValidationLayers)
    {
        validationLayers = System::IsExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) &&
                           System::IsLayerSupported("VK_LAYER_KHRONOS_validation");

        if (!validationLayers && m_RequireValidationLayers)
            return FormattedResult<Instance>::Error(
                VK_ERROR_LAYER_NOT_PRESENT,
                "Validation layers (along with the debug utils extension) are not suported");

        if (validationLayers && !contains(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        if (validationLayers && !contains(layers, "VK_LAYER_KHRONOS_validation"))
            layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    const bool properties2Support =
        apiVersion < VKIT_MAKE_VERSION(0, 1, 1, 0) &&
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

        const auto generateError = [](const char *p_Extension) -> FormattedResult<Instance> {
            return FormattedResult<Instance>::Error(VKIT_FORMAT_ERROR(
                VK_ERROR_EXTENSION_NOT_PRESENT,
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
#ifdef TKIT_ENABLE_ASSERTS
        msgInfo.pfnUserCallback = m_DebugCallback ? m_DebugCallback : defaultDebugCallback;
#else
        msgInfo.pfnUserCallback = m_DebugCallback;
#endif
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
    instanceInfo.enabledExtensionCount = extensions.size();
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    instanceInfo.enabledLayerCount = layers.size();
    instanceInfo.ppEnabledLayerNames = layers.data();
    instanceInfo.pNext = validationLayers ? &msgInfo : nullptr;
#ifdef VK_KHR_portability_enumeration
    if (portabilitySupport)
        instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkInstance vkinstance;
    VkResult result = vkCreateInstance(&instanceInfo, m_AllocationCallbacks, &vkinstance);
    if (result != VK_SUCCESS)
        return FormattedResult<Instance>::Error(result, "Failed to create the vulkan instance");

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    if (validationLayers)
    {
        const auto createDebugMessenger = System::GetInstanceFunction<PFN_vkCreateDebugUtilsMessengerEXT>(
            "vkCreateDebugUtilsMessengerEXT", vkinstance);
        if (!createDebugMessenger)
            return FormattedResult<Instance>::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                                    "Failed to get the vkCreateDebugUtilsMessengerEXT function");

        result = createDebugMessenger(vkinstance, &msgInfo, nullptr, &debugMessenger);
        if (result != VK_SUCCESS)
        {
            vkDestroyInstance(vkinstance, m_AllocationCallbacks);
            return FormattedResult<Instance>::Error(result, "Failed to create the debug messenger");
        }
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
    if (m_Headless)
        info.Flags |= Flag_Headless;
    if (validationLayers)
        info.Flags |= Flag_HasValidationLayers;
    if (properties2Support)
        info.Flags |= Flag_Properties2Extension;

    TKIT_ASSERT((validationLayers && debugMessenger) || (!validationLayers && !debugMessenger),
                "[VULKIT] The debug messenger must be available if validation layers are enabled");

    return FormattedResult<Instance>::Ok(vkinstance, info);
}

Instance::Instance(VkInstance p_Instance, const Info &p_Info) noexcept : m_Instance(p_Instance), m_Info(p_Info)
{
}

bool Instance::IsExtensionEnabled(const char *p_Extension) const noexcept
{
    return contains(m_Info.EnabledExtensions, p_Extension);
}
bool Instance::IsLayerEnabled(const char *p_Layer) const noexcept
{
    return contains(m_Info.EnabledLayers, p_Layer);
}

static void destroy(const VkInstance p_Instance, const Instance::Info &p_Info) noexcept
{
    TKIT_ASSERT(p_Instance, "[VULKIT] The vulkan instance is null, which probably means it has already been destroyed");
    // Should be already available if user managed to create instance

    if ((p_Info.Flags & Instance::Flag_HasValidationLayers) && p_Info.DebugMessenger)
    {
        const auto destroyDebugMessenger = System::GetInstanceFunction<PFN_vkDestroyDebugUtilsMessengerEXT>(
            "vkDestroyDebugUtilsMessengerEXT", p_Instance);
        TKIT_ASSERT(destroyDebugMessenger, "[VULKIT] Failed to get the vkDestroyDebugUtilsMessengerEXT function");
        destroyDebugMessenger(p_Instance, p_Info.DebugMessenger, p_Info.AllocationCallbacks);
    }
    vkDestroyInstance(p_Instance, p_Info.AllocationCallbacks);
}

void Instance::Destroy() noexcept
{
    destroy(m_Instance, m_Info);
    m_Instance = VK_NULL_HANDLE;
}

void Instance::SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
{
    const VkInstance instance = m_Instance;
    const Info info = m_Info;
    p_Queue.Push([instance, info]() { destroy(instance, info); });
}

VkInstance Instance::GetInstance() const noexcept
{
    return m_Instance;
}
const Instance::Info &Instance::GetInfo() const noexcept
{
    return m_Info;
}
Instance::operator VkInstance() const noexcept
{
    return m_Instance;
}
Instance::operator bool() const noexcept
{
    return m_Instance != VK_NULL_HANDLE;
}

Instance::Builder &Instance::Builder::SetApplicationName(const char *p_Name) noexcept
{
    m_ApplicationName = p_Name;
    return *this;
}
Instance::Builder &Instance::Builder::SetEngineName(const char *p_Name) noexcept
{
    m_EngineName = p_Name;
    return *this;
}
Instance::Builder &Instance::Builder::SetApplicationVersion(u32 p_Version) noexcept
{
    m_ApplicationVersion = p_Version;
    return *this;
}
Instance::Builder &Instance::Builder::SetEngineVersion(u32 p_Version) noexcept
{
    m_EngineVersion = p_Version;
    return *this;
}
Instance::Builder &Instance::Builder::SetApplicationVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept
{
    return SetApplicationVersion(VKIT_MAKE_VERSION(0, p_Major, p_Minor, p_Patch));
}
Instance::Builder &Instance::Builder::SetEngineVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept
{
    return SetEngineVersion(VKIT_MAKE_VERSION(0, p_Major, p_Minor, p_Patch));
}
Instance::Builder &Instance::Builder::RequireApiVersion(u32 p_Version) noexcept
{
    m_RequiredApiVersion = p_Version;
    if (m_RequestedApiVersion < m_RequiredApiVersion)
        m_RequestedApiVersion = m_RequiredApiVersion;
    return *this;
}
Instance::Builder &Instance::Builder::RequireApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept
{
    return RequireApiVersion(VKIT_MAKE_VERSION(0, p_Major, p_Minor, p_Patch));
}
Instance::Builder &Instance::Builder::RequestApiVersion(u32 p_Version) noexcept
{
    m_RequestedApiVersion = p_Version;
    if (m_RequestedApiVersion < m_RequiredApiVersion)
        m_RequiredApiVersion = m_RequestedApiVersion;
    return *this;
}
Instance::Builder &Instance::Builder::RequestApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept
{
    return RequestApiVersion(VKIT_MAKE_VERSION(0, p_Major, p_Minor, p_Patch));
}
Instance::Builder &Instance::Builder::RequireExtension(const char *p_Extension) noexcept
{
    m_RequiredExtensions.push_back(p_Extension);
    return *this;
}
Instance::Builder &Instance::Builder::RequireExtensions(TKit::Span<const char *const> p_Extensions) noexcept
{
    m_RequiredExtensions.insert(m_RequiredExtensions.end(), p_Extensions.begin(), p_Extensions.end());
    return *this;
}
Instance::Builder &Instance::Builder::RequestExtension(const char *p_Extension) noexcept
{
    m_RequestedExtensions.push_back(p_Extension);
    return *this;
}
Instance::Builder &Instance::Builder::RequestExtensions(TKit::Span<const char *const> p_Extensions) noexcept
{
    m_RequestedExtensions.insert(m_RequestedExtensions.end(), p_Extensions.begin(), p_Extensions.end());
    return *this;
}
Instance::Builder &Instance::Builder::RequireLayer(const char *p_Layer) noexcept
{
    m_RequiredLayers.push_back(p_Layer);
    return *this;
}
Instance::Builder &Instance::Builder::RequireLayers(TKit::Span<const char *const> p_Layers) noexcept
{
    m_RequiredLayers.insert(m_RequiredLayers.end(), p_Layers.begin(), p_Layers.end());
    return *this;
}
Instance::Builder &Instance::Builder::RequestLayer(const char *p_Layer) noexcept
{
    m_RequestedLayers.push_back(p_Layer);
    return *this;
}
Instance::Builder &Instance::Builder::RequestLayers(TKit::Span<const char *const> p_Layers) noexcept
{
    m_RequestedLayers.insert(m_RequestedLayers.end(), p_Layers.begin(), p_Layers.end());
    return *this;
}
Instance::Builder &Instance::Builder::RequireValidationLayers() noexcept
{
    m_RequireValidationLayers = true;
    m_RequestValidationLayers = true;
    return *this;
}
Instance::Builder &Instance::Builder::RequestValidationLayers() noexcept
{
    m_RequestValidationLayers = true;
    return *this;
}
Instance::Builder &Instance::Builder::SetDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT p_Callback) noexcept
{
    m_DebugCallback = p_Callback;
    return *this;
}
Instance::Builder &Instance::Builder::SetHeadless(bool p_Headless) noexcept
{
    m_Headless = p_Headless;
    return *this;
}
Instance::Builder &Instance::Builder::SetDebugMessengerUserData(void *p_Data) noexcept
{
    m_DebugMessengerUserData = p_Data;
    return *this;
}
Instance::Builder &Instance::Builder::SetAllocationCallbacks(
    const VkAllocationCallbacks *p_AllocationCallbacks) noexcept
{
    m_AllocationCallbacks = p_AllocationCallbacks;
    return *this;
}

} // namespace VKit