#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/backend/system.hpp"
#include "tkit/memory/ptr.hpp"
#include "tkit/utilities/result.hpp"
#include <vulkan/vulkan.hpp>

namespace VKit
{
enum InstanceFlags : u8
{
    InstanceFlags_Headless = 1 << 0,
    InstanceFlags_HasValidationLayers = 1 << 1,
    InstanceFlags_Properties2Extension = 1 << 2
};

class VKIT_API Instance : public TKit::RefCounted<Instance>
{
  public:
    struct Info
    {
        const char *ApplicationName;
        const char *EngineName;

        u32 ApplicationVersion;
        u32 EngineVersion;
        u32 ApiVersion;

        u8 Flags;

        DynamicArray<const char *> EnabledExtensions;
        DynamicArray<const char *> EnabledLayers;

        VkDebugUtilsMessengerEXT DebugMessenger;
        const VkAllocationCallbacks *AllocationCallbacks;
    };

    class Builder
    {
      public:
        FormattedResult<Instance> Build() const noexcept;

        Builder &SetApplicationName(const char *p_Name) noexcept;
        Builder &SetEngineName(const char *p_Name) noexcept;

        Builder &SetApplicationVersion(u32 p_Version) noexcept;
        Builder &SetEngineVersion(u32 p_Version) noexcept;

        Builder &SetApplicationVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;
        Builder &SetEngineVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;

        Builder &RequireApiVersion(u32 p_Version) noexcept;
        Builder &RequireApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;

        Builder &RequestApiVersion(u32 p_Version) noexcept;
        Builder &RequestApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;

        Builder &RequireExtension(const char *p_Extension) noexcept;
        Builder &RequireExtensions(std::span<const char *const> p_Extensions) noexcept;

        Builder &RequestExtension(const char *p_Extension) noexcept;
        Builder &RequestExtensions(std::span<const char *const> p_Extensions) noexcept;

        Builder &RequireLayer(const char *p_Layer) noexcept;
        Builder &RequireLayers(std::span<const char *const> p_Layers) noexcept;

        Builder &RequestLayer(const char *p_Layer) noexcept;
        Builder &RequestLayers(std::span<const char *const> p_Layers) noexcept;

        Builder &RequireValidationLayers() noexcept;
        Builder &RequestValidationLayers() noexcept;

        Builder &SetDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT p_Callback) noexcept;
        Builder &SetHeadless(bool p_Headless = true) noexcept;

        Builder &SetDebugMessengerUserData(void *p_Data) noexcept;
        Builder &SetAllocationCallbacks(const VkAllocationCallbacks *p_AllocationCallbacks) noexcept;

      private:
        const char *m_ApplicationName = nullptr;
        const char *m_EngineName = nullptr;

        u32 m_ApplicationVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_EngineVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_RequiredApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_RequestedApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);

        DynamicArray<const char *> m_RequiredExtensions;
        DynamicArray<const char *> m_RequestedExtensions;

        DynamicArray<const char *> m_RequiredLayers;
        DynamicArray<const char *> m_RequestedLayers;

        bool m_RequireValidationLayers = false;
        bool m_RequestValidationLayers = false;
        bool m_Headless = false;

        void *m_DebugMessengerUserData = nullptr;
        const VkAllocationCallbacks *m_AllocationCallbacks = nullptr;

        PFN_vkDebugUtilsMessengerCallbackEXT m_DebugCallback = nullptr;
    };

    Instance(VkInstance p_Instance, const Info &p_Info) noexcept;

    bool IsExtensionEnabled(const char *p_Extension) const noexcept;
    bool IsLayerEnabled(const char *p_Layer) const noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    VkInstance GetInstance() const noexcept;
    const Info &GetInfo() const noexcept;

    explicit(false) operator VkInstance() const noexcept;
    explicit(false) operator bool() const noexcept;

    template <typename F> F GetFunction(const char *p_Name) const noexcept
    {
        return reinterpret_cast<F>(vkGetInstanceProcAddr(m_Instance, p_Name));
    }

  private:
    VkInstance m_Instance;
    Info m_Info;
};
} // namespace VKit