#include "vkit/core/pch.hpp"
#include "vkit/backend/system.hpp"

namespace VKit
{
VulkanResult::VulkanResult(VkResult p_Result, std::string_view p_Message) noexcept
    : Result(p_Result), Message(p_Message)
{
}

VulkanResult VulkanResult::Success() noexcept
{
    return VulkanResult{};
}
VulkanResult VulkanResult::Error(VkResult p_Result, std::string_view p_Message) noexcept
{
    return VulkanResult(p_Result, p_Message);
}

VulkanResult::operator bool() const noexcept
{
    return Result == VK_SUCCESS;
}

VulkanResult System::Initialize() noexcept
{
    const auto enumerateExtensions =
        GetInstanceFunction<PFN_vkEnumerateInstanceExtensionProperties>("vkEnumerateInstanceExtensionProperties");
    if (!enumerateExtensions)
        return VKIT_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT,
                          "Failed to get the vkEnumerateInstanceExtensionProperties function");

    u32 extensionCount = 0;
    VkResult result;
    result = enumerateExtensions(nullptr, &extensionCount, nullptr);
    if (result != VK_SUCCESS)
        return VKIT_ERROR(result, "Failed to get the number of instance extensions");

    AvailableExtensions.resize(extensionCount);
    result = enumerateExtensions(nullptr, &extensionCount, AvailableExtensions.data());
    if (result != VK_SUCCESS)
        return VKIT_ERROR(result, "Failed to get the instance extensions");

    const auto enumerateLayers =
        GetInstanceFunction<PFN_vkEnumerateInstanceLayerProperties>("vkEnumerateInstanceLayerProperties");
    if (!enumerateLayers)
        return VKIT_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT,
                          "Failed to get the vkEnumerateInstanceLayerProperties function");

    u32 layerCount = 0;
    result = enumerateLayers(&layerCount, nullptr);
    if (result != VK_SUCCESS)
        return VKIT_ERROR(result, "Failed to get the number of instance layers");

    AvailableLayers.resize(layerCount);
    result = enumerateLayers(&layerCount, AvailableLayers.data());
    if (result != VK_SUCCESS)
        return VKIT_ERROR(result, "Failed to get the instance layers");

    return VulkanResult::Success();
}

const VkExtensionProperties *System::GetExtension(const char *p_Name) noexcept
{
    for (const VkExtensionProperties &extension : AvailableExtensions)
        if (strcmp(p_Name, extension.extensionName) == 0)
            return &extension;
    return nullptr;
}

const VkLayerProperties *System::GetLayer(const char *p_Name) noexcept
{
    for (const VkLayerProperties &layer : AvailableLayers)
        if (strcmp(p_Name, layer.layerName) == 0)
            return &layer;
    return nullptr;
}

bool System::IsExtensionSupported(const char *p_Name) noexcept
{
    for (const VkExtensionProperties &extension : AvailableExtensions)
        if (strcmp(p_Name, extension.extensionName) == 0)
            return true;
    return false;
}

bool System::IsLayerSupported(const char *p_Name) noexcept
{
    for (const VkLayerProperties &layer : AvailableLayers)
        if (strcmp(p_Name, layer.layerName) == 0)
            return true;
    return false;
}

bool System::AreExtensionsSupported(std::span<const char *const> p_Names) noexcept
{
    for (const char *name : p_Names)
        if (!IsExtensionSupported(name))
            return false;
    return true;
}
bool System::AreLayersSupported(std::span<const char *const> p_Names) noexcept
{
    for (const char *name : p_Names)
        if (!IsLayerSupported(name))
            return false;
    return true;
}

void DeletionQueue::Push(std::function<void()> &&p_Deleter) noexcept
{
    m_Deleters.push_back(std::move(p_Deleter));
}
void DeletionQueue::Flush() noexcept
{
    for (auto it = m_Deleters.rbegin(); it != m_Deleters.rend(); ++it)
        (*it)();
    m_Deleters.clear();
}

} // namespace VKit