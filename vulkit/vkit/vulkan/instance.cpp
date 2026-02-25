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

static const char *toString(const VkObjectType type)
{
    switch (type)
    {
    case VK_OBJECT_TYPE_UNKNOWN:
        return "VK_OBJECT_TYPE_UNKNOWN";
    case VK_OBJECT_TYPE_INSTANCE:
        return "VK_OBJECT_TYPE_INSTANCE";
    case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
        return "VK_OBJECT_TYPE_PHYSICAL_DEVICE";
    case VK_OBJECT_TYPE_DEVICE:
        return "VK_OBJECT_TYPE_DEVICE";
    case VK_OBJECT_TYPE_QUEUE:
        return "VK_OBJECT_TYPE_QUEUE";
    case VK_OBJECT_TYPE_SEMAPHORE:
        return "VK_OBJECT_TYPE_SEMAPHORE";
    case VK_OBJECT_TYPE_COMMAND_BUFFER:
        return "VK_OBJECT_TYPE_COMMAND_BUFFER";
    case VK_OBJECT_TYPE_FENCE:
        return "VK_OBJECT_TYPE_FENCE";
    case VK_OBJECT_TYPE_DEVICE_MEMORY:
        return "VK_OBJECT_TYPE_DEVICE_MEMORY";
    case VK_OBJECT_TYPE_BUFFER:
        return "VK_OBJECT_TYPE_BUFFER";
    case VK_OBJECT_TYPE_IMAGE:
        return "VK_OBJECT_TYPE_IMAGE";
    case VK_OBJECT_TYPE_EVENT:
        return "VK_OBJECT_TYPE_EVENT";
    case VK_OBJECT_TYPE_QUERY_POOL:
        return "VK_OBJECT_TYPE_QUERY_POOL";
    case VK_OBJECT_TYPE_BUFFER_VIEW:
        return "VK_OBJECT_TYPE_BUFFER_VIEW";
    case VK_OBJECT_TYPE_IMAGE_VIEW:
        return "VK_OBJECT_TYPE_IMAGE_VIEW";
    case VK_OBJECT_TYPE_SHADER_MODULE:
        return "VK_OBJECT_TYPE_SHADER_MODULE";
    case VK_OBJECT_TYPE_PIPELINE_CACHE:
        return "VK_OBJECT_TYPE_PIPELINE_CACHE";
    case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
        return "VK_OBJECT_TYPE_PIPELINE_LAYOUT";
    case VK_OBJECT_TYPE_RENDER_PASS:
        return "VK_OBJECT_TYPE_RENDER_PASS";
    case VK_OBJECT_TYPE_PIPELINE:
        return "VK_OBJECT_TYPE_PIPELINE";
    case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
        return "VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT";
    case VK_OBJECT_TYPE_SAMPLER:
        return "VK_OBJECT_TYPE_SAMPLER";
    case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
        return "VK_OBJECT_TYPE_DESCRIPTOR_POOL";
    case VK_OBJECT_TYPE_DESCRIPTOR_SET:
        return "VK_OBJECT_TYPE_DESCRIPTOR_SET";
    case VK_OBJECT_TYPE_FRAMEBUFFER:
        return "VK_OBJECT_TYPE_FRAMEBUFFER";
    case VK_OBJECT_TYPE_COMMAND_POOL:
        return "VK_OBJECT_TYPE_COMMAND_POOL";
#ifdef VKIT_API_VERSION_1_1
    case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:
        return "VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE";
    case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:
        return "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION";
#endif
#ifdef VKIT_API_VERSION_1_3
    case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT:
        return "VK_OBJECT_TYPE_PRIVATE_DATA_SLOT";
#endif
#ifdef VK_KHR_surface
    case VK_OBJECT_TYPE_SURFACE_KHR:
        return "VK_OBJECT_TYPE_SURFACE_KHR";
#endif
#ifdef VK_KHR_swapchain
    case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
        return "VK_OBJECT_TYPE_SWAPCHAIN_KHR";
#endif
#ifdef VK_KHR_display
    case VK_OBJECT_TYPE_DISPLAY_KHR:
        return "VK_OBJECT_TYPE_DISPLAY_KHR";
    case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
        return "VK_OBJECT_TYPE_DISPLAY_MODE_KHR";
#endif
#ifdef VK_EXT_debug_report
    case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
        return "VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT";
#endif
#ifdef VK_KHR_video_queue
    case VK_OBJECT_TYPE_VIDEO_SESSION_KHR:
        return "VK_OBJECT_TYPE_VIDEO_SESSION_KHR";
    case VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR:
        return "VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR";
#endif
#ifdef VK_NVX_binary_import
    case VK_OBJECT_TYPE_CU_MODULE_NVX:
        return "VK_OBJECT_TYPE_CU_MODULE_NVX";
    case VK_OBJECT_TYPE_CU_FUNCTION_NVX:
        return "VK_OBJECT_TYPE_CU_FUNCTION_NVX";
#endif
#ifdef VK_EXT_debug_utils
    case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
        return "VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT";
#endif
#ifdef VK_KHR_acceleration_structure
    case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:
        return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR";
#endif
#ifdef VK_EXT_validation_cache
    case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:
        return "VK_OBJECT_TYPE_VALIDATION_CACHE_EXT";
#endif
#ifdef VK_NV_ray_tracing
    case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV:
        return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV";
#endif
#ifdef VK_INTEL_performance_query
    case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL:
        return "VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL";
#endif
#ifdef VK_KHR_deferred_host_operations
    case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR:
        return "VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR";
#endif
#ifdef VK_NV_device_generated_commands
    case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV:
        return "VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    case VK_OBJECT_TYPE_CUDA_MODULE_NV:
        return "VK_OBJECT_TYPE_CUDA_MODULE_NV";
    case VK_OBJECT_TYPE_CUDA_FUNCTION_NV:
        return "VK_OBJECT_TYPE_CUDA_FUNCTION_NV";
#endif
#ifdef VK_FUCHSIA_buffer_collection
    case VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA:
        return "VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA";
#endif
#ifdef VK_EXT_opacity_micromap
    case VK_OBJECT_TYPE_MICROMAP_EXT:
        return "VK_OBJECT_TYPE_MICROMAP_EXT";
#endif
#ifdef VK_ARM_tensors
    case VK_OBJECT_TYPE_TENSOR_ARM:
        return "VK_OBJECT_TYPE_TENSOR_ARM";
    case VK_OBJECT_TYPE_TENSOR_VIEW_ARM:
        return "VK_OBJECT_TYPE_TENSOR_VIEW_ARM";
#endif
#ifdef VK_NV_optical_flow
    case VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV:
        return "VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV";
#endif
#ifdef VK_EXT_shader_object
    case VK_OBJECT_TYPE_SHADER_EXT:
        return "VK_OBJECT_TYPE_SHADER_EXT";
#endif
#ifdef VK_KHR_pipeline_binary
    case VK_OBJECT_TYPE_PIPELINE_BINARY_KHR:
        return "VK_OBJECT_TYPE_PIPELINE_BINARY_KHR";
#endif
#ifdef VK_ARM_data_graph
    case VK_OBJECT_TYPE_DATA_GRAPH_PIPELINE_SESSION_ARM:
        return "VK_OBJECT_TYPE_DATA_GRAPH_PIPELINE_SESSION_ARM";
#endif
#ifdef VK_NV_external_compute_queue
    case VK_OBJECT_TYPE_EXTERNAL_COMPUTE_QUEUE_NV:
        return "VK_OBJECT_TYPE_EXTERNAL_COMPUTE_QUEUE_NV";
#endif
#ifdef VK_EXT_device_generated_commands
    case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_EXT:
        return "VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_EXT";
    case VK_OBJECT_TYPE_INDIRECT_EXECUTION_SET_EXT:
        return "VK_OBJECT_TYPE_INDIRECT_EXECUTION_SET_EXT";
#endif
    default:
        return "VK_OBJECT_TYPE_UNKNOWN";
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL defaultDebugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                           const VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
                                                           void *)
{
    const char *mtype = toString(messageType);
#define PRINT_DEBUG_INFO(fn)                                                                                           \
    fn("[VULKIT][{}] Id={} Code={:x} Message={}", mtype,                                                               \
       callbackData->pMessageIdName ? callbackData->pMessageIdName : "?", callbackData->messageIdNumber,               \
       callbackData->pMessage);                                                                                        \
    if (callbackData->objectCount != 0)                                                                                \
    {                                                                                                                  \
        fn("[VULKIT][{}] Objects ({}):", mtype, callbackData->objectCount);                                            \
        for (u32 i = 0; i < callbackData->objectCount; ++i)                                                            \
        {                                                                                                              \
            const VkDebugUtilsObjectNameInfoEXT &obj = callbackData->pObjects[i];                                      \
            fn("[VULKIT][{}]    object[{}] = {}", mtype, i, toString(obj.objectType));                                 \
            if (obj.pObjectName)                                                                                       \
            {                                                                                                          \
                fn("[VULKIT][{}]        Name=\"{}\"", mtype, obj.pObjectName);                                         \
            }                                                                                                          \
            fn("[VULKIT][{}]        Handle={:x}", mtype, obj.objectHandle);                                            \
        }                                                                                                              \
    }                                                                                                                  \
    if (callbackData->queueLabelCount != 0)                                                                            \
    {                                                                                                                  \
        fn("[VULKIT][{}] Queue label stack ({}):", mtype, callbackData->queueLabelCount);                              \
        for (u32 i = 0; i < callbackData->queueLabelCount; ++i)                                                        \
        {                                                                                                              \
            fn("[VULKIT][{}]    label[{}] -> {}", mtype, i, callbackData->pQueueLabels[i].pLabelName);                 \
        }                                                                                                              \
    }                                                                                                                  \
    if (callbackData->cmdBufLabelCount != 0)                                                                           \
    {                                                                                                                  \
        fn("[VULKIT][{}] CmdBuf label stack ({}):", mtype, callbackData->queueLabelCount);                             \
        for (u32 i = 0; i < callbackData->cmdBufLabelCount; ++i)                                                       \
        {                                                                                                              \
            fn("[VULKIT][{}]    label[{}] -> {}", mtype, i, callbackData->pCmdBufLabels[i].pLabelName);                \
        }                                                                                                              \
    }

    switch (severity)
    {
#ifdef TKIT_ENABLE_DEBUG_LOGS
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
        PRINT_DEBUG_INFO(TKIT_LOG_DEBUG);
        return VK_FALSE;
    }
#endif
#ifdef TKIT_ENABLE_INFO_LOGS
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        PRINT_DEBUG_INFO(TKIT_LOG_INFO);
        return VK_FALSE;
#endif
#ifdef TKIT_ENABLE_WARNING_LOGS
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        PRINT_DEBUG_INFO(TKIT_LOG_WARNING);
        return VK_FALSE;
#endif
#ifdef TKIT_ENABLE_ASSERTS
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        PRINT_DEBUG_INFO(TKIT_FATAL);
        return VK_FALSE;
#elif defined(TKIT_ENABLE_ERROR_LOGS)
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        PRINT_DEBUG_INFO(TKIT_LOG_ERROR);
        return VK_FALSE;
#endif
    default:
        return VK_FALSE;
    }
#undef PRINT_DEBUG_INFO
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

    TKit::StackArray<const char *> extensions;
    extensions.Reserve(m_RequiredExtensions.GetCapacity());
    for (const char *extension : m_RequiredExtensions)
        if (!Core::IsExtensionSupported(extension))
            return Result<Instance>::Error(
                Error_MissingExtension,
                TKit::Format("[VULKIT][INSTANCE] The required extension '{}' is not suported", extension));
        else if (!contains(extensions, extension))
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
        if (!Core::IsLayerSupported(layer))
            return Result<Instance>::Error(
                Error_MissingLayer, TKit::Format("[VULKIT][INSTANCE] The required layer '{}' is not suported", layer));
        else if (!contains(layers, layer))
            layers.Append(layer);

    for (const char *layer : m_RequestedLayers)
    {
        const bool supported = Core::IsLayerSupported(layer);
        TKIT_LOG_WARNING_IF(!supported, "[VULKIT][INSTANCE] The requested layer '{}' is not suported", layer);
        if (supported && !contains(layers, layer))
            layers.Append(layer);
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

    const void *pNext = nullptr;

#ifdef VK_EXT_debug_utils
    VkDebugUtilsMessengerCreateInfoEXT msgInfo{};
    const bool hasDebugUtils = contains(extensions, "VK_EXT_debug_utils");
    if (hasDebugUtils)
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
        TKIT_LOG_WARNING_IF(m_DebugMessengerUserData && !m_DebugCallback,
                            "[VULKIT][INSTANCE] Debug messenger data was provided but no custom debug callback was "
                            "passed, defaulting to the builtin's vulkit callback which does not use user data. The "
                            "data will be ignored");
        pNext = &msgInfo;
    }
    TKIT_LOG_WARNING_IF(!hasDebugUtils && m_DebugCallback,
                        "[VULKIT][INSTANCE] A debug messenger callback was provided but 'VK_EXT_debug_utils' extension "
                        "was not enabled or supported. The callback will be ignored");
    TKIT_LOG_WARNING_IF(!hasDebugUtils && m_DebugMessengerUserData,
                        "[VULKIT][INSTANCE] Debug messenger data was provided but 'VK_EXT_debug_utils' extension "
                        "was not enabled or supported. The data will be ignored");
#endif

#ifdef VK_EXT_validation_features
    VkValidationFeaturesEXT valFeatures{};
    bool hasValFeatures = contains(extensions, "VK_EXT_validation_features");
    if (hasValFeatures && (!m_EnabledValFeatures.IsEmpty() || !m_DisabledValFeatures.IsEmpty()))
    {
        valFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        if (!m_EnabledValFeatures.IsEmpty())
        {
            valFeatures.pEnabledValidationFeatures = m_EnabledValFeatures.GetData();
            valFeatures.enabledValidationFeatureCount = m_EnabledValFeatures.GetSize();
        }
        if (!m_DisabledValFeatures.IsEmpty())
        {
            valFeatures.pDisabledValidationFeatures = m_DisabledValFeatures.GetData();
            valFeatures.disabledValidationFeatureCount = m_DisabledValFeatures.GetSize();
        }
#    ifdef VK_EXT_debug_utils
        if (hasDebugUtils)
            valFeatures.pNext = &msgInfo;
#    endif
        pNext = &valFeatures;
    }

    TKIT_LOG_WARNING_IF(hasValFeatures && m_EnabledValFeatures.IsEmpty() && m_DisabledValFeatures.IsEmpty(),
                        "[VULKIT][INSTANCE] The extension 'VK_EXT_validation_features' was enabled, but no features "
                        "were requested to be enabled or disabled");
    TKIT_LOG_WARNING_IF(
        !hasValFeatures && !m_EnabledValFeatures.IsEmpty(),
        "[VULKIT][INSTANCE] Validation features were requested to be enabled but the 'VK_EXT_validation_features' "
        "extension was not enabled or supported. Request will be ignored");
    TKIT_LOG_WARNING_IF(
        !hasValFeatures && !m_DisabledValFeatures.IsEmpty(),
        "[VULKIT][INSTANCE] Validation features were requested to be disabled but the 'VK_EXT_validation_features' "
        "extension was not enabled or supported. Request will be ignored");

    hasValFeatures &= !m_EnabledValFeatures.IsEmpty() || !m_DisabledValFeatures.IsEmpty();
#endif

#ifdef VK_EXT_layer_settings
    VkLayerSettingsCreateInfoEXT layerSettings{};
    const bool hasSettings = contains(extensions, "VK_EXT_layer_settings");
    if (hasSettings && !m_LayerSettings.IsEmpty())
    {
        layerSettings.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
        layerSettings.pSettings = m_LayerSettings.GetData();
        layerSettings.settingCount = m_LayerSettings.GetSize();
#    if defined(VK_EXT_validation_features) || defined(VK_EXT_debug_utils)
        if (hasValFeatures)
            layerSettings.pNext = &valFeatures;
        else if (hasDebugUtils)
            layerSettings.pNext = &msgInfo;
#    elif defined(VK_EXT_validation_features)
        if (hasValFeatures)
            layerSettings.pNext = &valFeatures;
#    elif defined(VK_EXT_debug_utils)
        if (hasDebugUtils)
            layerSettings.pNext = &msgInfo;
#    endif

        pNext = &layerSettings;
    }
    TKIT_LOG_WARNING_IF(hasSettings && m_LayerSettings.IsEmpty(),
                        "[VULKIT][INSTANCE] The extension 'VK_EXT_layer_settings' was enabled, but no settings "
                        "were requested");
    TKIT_LOG_WARNING_IF(!hasSettings && !m_LayerSettings.IsEmpty(),
                        "[VULKAN][INSTANCE] Layer settings were requested but the extension 'VK_EXT_layer_settings' "
                        "was not enabled or supported. The settings will be ignored");
#endif

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
    instanceInfo.pNext = pNext;
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

#ifdef VK_EXT_debug_utils
    if (hasDebugUtils)
    {
        const auto cleanup = [&] {
            table->DestroyDebugUtilsMessengerEXT(vkinstance, debugMessenger, m_AllocationCallbacks);
            table->DestroyInstance(vkinstance, m_AllocationCallbacks);
            alloc->Destroy(table);
        };
        VKIT_RETURN_IF_FAILED(
            table->CreateDebugUtilsMessengerEXT(vkinstance, &msgInfo, m_AllocationCallbacks, &debugMessenger),
            Result<Instance>, cleanup());
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
    info.Headless = m_Headless;

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

#ifdef VK_EXT_debug_utils
    if (m_Info.DebugMessenger)
    {
        m_Info.Table->DestroyDebugUtilsMessengerEXT(m_Instance, m_Info.DebugMessenger, m_Info.AllocationCallbacks);
        m_Info.DebugMessenger = VK_NULL_HANDLE;
    }
#endif

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
#ifdef VK_EXT_debug_utils
Instance::Builder &Instance::Builder::SetDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT callback)
{
    m_DebugCallback = callback;
    return *this;
}
Instance::Builder &Instance::Builder::SetDebugMessengerUserData(void *data)
{
    m_DebugMessengerUserData = data;
    return *this;
}
#endif
#ifdef VK_EXT_validation_features
Instance::Builder &Instance::Builder::SetValidationFeature(const VkValidationFeatureEnableEXT enable)
{
    m_EnabledValFeatures.Append(enable);
    return *this;
}
Instance::Builder &Instance::Builder::SetValidationFeature(const VkValidationFeatureDisableEXT disable)
{
    m_DisabledValFeatures.Append(disable);
    return *this;
}
#endif
#ifdef VK_EXT_layer_settings
Instance::Builder &Instance::Builder::AddLayerSetting(const VkLayerSettingEXT setting)
{
    m_LayerSettings.Append(setting);
    return *this;
}
#endif
Instance::Builder &Instance::Builder::SetHeadless(bool headless)
{
    m_Headless = headless;
    return *this;
}
Instance::Builder &Instance::Builder::SetAllocationCallbacks(const VkAllocationCallbacks *allocationCallbacks)
{
    m_AllocationCallbacks = allocationCallbacks;
    return *this;
}

} // namespace VKit
