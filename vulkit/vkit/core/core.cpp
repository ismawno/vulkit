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

u8 s_PushedAlloc = 0;
Allocation s_Allocation{};

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
        return Result<>::Error(
            Error_VulkanLibraryNotFound,
            "[VULKIT][LOADER] Failed to load the vulkan library. All attempts have been exhausted. You may try "
            "specifying a custom path for it");

    Vulkan::Load(s_Library);

    // these are purposefully leaked
    if (p_Specs.Allocators.Arena)
        s_Allocation.Arena = p_Specs.Allocators.Arena;
    else if (!s_Allocation.Arena)
        s_Allocation.Arena = new TKit::ArenaAllocator(4_mib);

    if (TKit::Memory::GetArena() != s_Allocation.Arena)
    {
        TKit::Memory::PushArena(s_Allocation.Arena);
        s_PushedAlloc |= 1 << 0;
    }

    if (p_Specs.Allocators.Stack)
        s_Allocation.Stack = p_Specs.Allocators.Stack;
    else if (!s_Allocation.Stack)
        s_Allocation.Stack = new TKit::StackAllocator(4_mib);

    if (TKit::Memory::GetStack() != s_Allocation.Stack)
    {
        TKit::Memory::PushStack(s_Allocation.Stack);
        s_PushedAlloc |= 1 << 1;
    }

    if (p_Specs.Allocators.Tier)
        s_Allocation.Tier = p_Specs.Allocators.Tier;
    else if (!s_Allocation.Tier)
        s_Allocation.Tier = new TKit::TierAllocator(64, 256_kib);

    if (TKit::Memory::GetTier() != s_Allocation.Tier)
    {
        TKit::Memory::PushTier(s_Allocation.Tier);
        s_PushedAlloc |= 1 << 2;
    }

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
    s_Capabilities = {};
    if (s_PushedAlloc & 4)
        TKit::Memory::PopTier();
    if (s_PushedAlloc & 2)
        TKit::Memory::PopStack();
    if (s_PushedAlloc & 1)
        TKit::Memory::PopArena();
}
} // namespace VKit::Core
