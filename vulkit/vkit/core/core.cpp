#include "vkit/core/pch.hpp"
#include "vkit/core/core.hpp"
#include "vkit/vulkan/loader.hpp"
#include "vkit/core/alias.hpp"

namespace VKit::Core
{

struct Capabilities
{
    TKit::ArenaArray<VkExtensionProperties> AvailableExtensions{};
    TKit::ArenaArray<VkLayerProperties> AvailableLayers{};
};

static Capabilities s_Capabilities{};

const VkExtensionProperties *GetExtensionByName(const char *p_Name)
{
    for (const VkExtensionProperties &extension : s_Capabilities.AvailableExtensions)
        if (strcmp(p_Name, extension.extensionName) == 0)
            return &extension;
    return nullptr;
}

const VkLayerProperties *GetLayerByName(const char *p_Name)
{
    for (const VkLayerProperties &layer : s_Capabilities.AvailableLayers)
        if (strcmp(p_Name, layer.layerName) == 0)
            return &layer;
    return nullptr;
}

bool IsExtensionSupported(const char *p_Name)
{
    for (const VkExtensionProperties &extension : s_Capabilities.AvailableExtensions)
        if (strcmp(p_Name, extension.extensionName) == 0)
            return true;
    return false;
}

bool IsLayerSupported(const char *p_Name)
{
    for (const VkLayerProperties &layer : s_Capabilities.AvailableLayers)
        if (strcmp(p_Name, layer.layerName) == 0)
            return true;
    return false;
}
const VkExtensionProperties &GetExtensionByIndex(const u32 p_Index)
{
    return s_Capabilities.AvailableExtensions[p_Index];
}
const VkLayerProperties &GetLayerByIndex(const u32 p_Index)
{
    return s_Capabilities.AvailableLayers[p_Index];
}

u32 GetExtensionCount()
{
    return s_Capabilities.AvailableExtensions.GetSize();
}
u32 GetLayerCount()
{
    return s_Capabilities.AvailableLayers.GetSize();
}
} // namespace VKit::Core

#if defined(TKIT_OS_APPLE) || defined(TKIT_OS_LINUX)
#    include <dlfcn.h>
#elif defined(TKIT_OS_WINDOWS)
#    include "tkit/core/windows.hpp"
#else
#    error "[VULKIT] Unsupported platform to load Vulkan library"
#endif

namespace VKit::Core
{
static void *s_Library = nullptr;
static void attempt(const char *p_LoaderPath)
{
    if (s_Library)
        return;

    TKIT_LOG_INFO("[VULKIT] Attempting to load vulkan library. Trying: {}", p_LoaderPath);
#ifdef TKIT_OS_WINDOWS
    s_Library = reinterpret_cast<void *>(LoadLibraryA(p_LoaderPath));
#else
    s_Library = dlopen(p_LoaderPath, RTLD_NOW | RTLD_LOCAL);
#endif

    TKIT_LOG_INFO_IF(s_Library, "[VULKIT] Success");
    TKIT_LOG_WARNING_IF(!s_Library, "[VULKIT] Failed");
}

static Specs s_Specs{};
static TKit::FixedArray<u8, TKIT_MAX_THREADS> s_ProvidedAllocators{};

Result<> Initialize(const Specs &p_Specs)
{
    if (s_Library)
        return Result<>::Ok();

    if (p_Specs.LoaderPath)
        attempt(p_Specs.LoaderPath);
#if defined(TKIT_OS_APPLE)
    attempt("libvulkan.dylib");
    attempt("libvulkan.1.dylib");
    attempt("@executable_path/../Frameworks/libvulkan.dylib");
    attempt("@executable_path/../Frameworks/libvulkan.1.dylib");
    attempt("/usr/local/lib/libvulkan.dylib");
    attempt("/usr/local/lib/libvulkan.1.dylib");
    attempt("/opt/homebrew/lib/libvulkan.dylib");
    attempt("/opt/homebrew/lib/libvulkan.1.dylib");

    attempt("libMoltenVK.dylib");
    attempt("@executable_path/../Frameworks/libMoltenVK.dylib");
    attempt("/usr/local/lib/libMoltenVK.dylib");
    attempt("/opt/homebrew/lib/libMoltenVK.dylib");
    attempt("/Library/Frameworks/Vulkan.framework/Vulkan");

#elif defined(TKIT_OS_LINUX)
    attempt("libvulkan.so");
    attempt("libvulkan.so.1");
#else
    attempt("vulkan-1.dll");
    attempt("vulkan.dll");
#endif
    if (!s_Library)
        return Result<>::Error(Error_VulkanLibraryNotFound,
                               "Failed to load the vulkan library. All attempts have been exhausted. You may try "
                               "specifying a custom path for it");

    Vulkan::Load(s_Library);

    s_Specs = p_Specs;
    for (u32 i = 0; i < TKIT_MAX_THREADS; ++i)
    {
        s_ProvidedAllocators[i] = 0;
        Allocation &alloc = s_Specs.Allocators[i];
        if (alloc.Arena)
            s_ProvidedAllocators[i] |= 1 << 0;
        else
            alloc.Arena = new TKit::ArenaAllocator{static_cast<u32>(i == 0 ? 4_mib : 4_kib)};

        if (alloc.Stack)
            s_ProvidedAllocators[i] |= 1 << 1;
        else
            alloc.Stack = new TKit::StackAllocator{static_cast<u32>(i == 0 ? 4_mib : 4_kib)};

        if (alloc.Tier)
            s_ProvidedAllocators[i] |= 1 << 2;
        else
            alloc.Tier = new TKit::TierAllocator{alloc.Arena, 64, static_cast<u32>(i == 0 ? 4_mib : 4_kib)};
    }
    TKit::Memory::PushArena(s_Specs.Allocators[0].Arena);
    TKit::Memory::PushStack(s_Specs.Allocators[0].Stack);
    TKit::Memory::PushTier(s_Specs.Allocators[0].Tier);

    u32 extensionCount = 0;
    VkResult result;
    result = Vulkan::EnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);

    s_Capabilities.AvailableExtensions.Resize(extensionCount);
    result = Vulkan::EnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                                          s_Capabilities.AvailableExtensions.GetData());
    if (result != VK_SUCCESS)
        return Result<>::Error(result);

    u32 layerCount = 0;
    result = Vulkan::EnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (result != VK_SUCCESS)
        return Result<>::Error(result);

    s_Capabilities.AvailableLayers.Resize(layerCount);
    result = Vulkan::EnumerateInstanceLayerProperties(&layerCount, s_Capabilities.AvailableLayers.GetData());
    if (result != VK_SUCCESS)
        return Result<>::Error(result);

    return Result<>::Ok();
}
void Terminate()
{
    if (!s_Library)
        return;
#if defined(TKIT_OS_APPLE) || defined(TKIT_OS_LINUX)
    dlclose(s_Library);
#else
    FreeLibrary(reinterpret_cast<HMODULE>(s_Library));
#endif
    s_Library = nullptr;
    TKit::Memory::PopArena();
    TKit::Memory::PopStack();
    TKit::Memory::PopTier();
    s_Capabilities = {};
    for (u32 i = 0; i < TKIT_MAX_THREADS; ++i)
    {
        if (!(s_ProvidedAllocators[i] & 1))
            delete s_Specs.Allocators[i].Arena;
        if (!(s_ProvidedAllocators[i] & 2))
            delete s_Specs.Allocators[i].Stack;
        if (!(s_ProvidedAllocators[i] & 4))
            delete s_Specs.Allocators[i].Tier;
    }
}
} // namespace VKit::Core
