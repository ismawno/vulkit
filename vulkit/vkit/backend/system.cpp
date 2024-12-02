#include "vkit/core/pch.hpp"
#include "vkit/backend/system.hpp"

namespace VKit
{
template <String MessageType>
VulkanResultInfo<MessageType>::VulkanResultInfo(VkResult p_Result, const MessageType &p_Message) noexcept
    : Result(p_Result), Message(p_Message)
{
}

template <String MessageType> VulkanResultInfo<MessageType> VulkanResultInfo<MessageType>::Success() noexcept
{
    return VulkanResultInfo{};
}
template <String MessageType>
VulkanResultInfo<MessageType> VulkanResultInfo<MessageType>::Error(VkResult p_Result,
                                                                   const MessageType &p_Message) noexcept
{
    return VulkanResultInfo(p_Result, p_Message);
}

template <String MessageType> VulkanResultInfo<MessageType>::operator bool() const noexcept
{
    return Result == VK_SUCCESS;
}

template class VulkanResultInfo<const char *>;
template class VulkanResultInfo<std::string>;

VulkanResult System::Initialize() noexcept
{
    const auto enumerateExtensions =
        GetInstanceFunction<PFN_vkEnumerateInstanceExtensionProperties>("vkEnumerateInstanceExtensionProperties");
    if (!enumerateExtensions)
        return VulkanResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                   "Failed to get the vkEnumerateInstanceExtensionProperties function");

    u32 extensionCount = 0;
    VkResult result;
    result = enumerateExtensions(nullptr, &extensionCount, nullptr);
    if (result != VK_SUCCESS)
        return VulkanResult::Error(result, "Failed to get the number of instance extensions");

    AvailableExtensions.resize(extensionCount);
    result = enumerateExtensions(nullptr, &extensionCount, AvailableExtensions.data());
    if (result != VK_SUCCESS)
        return VulkanResult::Error(result, "Failed to get the instance extensions");

    const auto enumerateLayers =
        GetInstanceFunction<PFN_vkEnumerateInstanceLayerProperties>("vkEnumerateInstanceLayerProperties");
    if (!enumerateLayers)
        return VulkanResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                   "Failed to get the vkEnumerateInstanceLayerProperties function");

    u32 layerCount = 0;
    result = enumerateLayers(&layerCount, nullptr);
    if (result != VK_SUCCESS)
        return VulkanResult::Error(result, "Failed to get the number of instance layers");

    AvailableLayers.resize(layerCount);
    result = enumerateLayers(&layerCount, AvailableLayers.data());
    if (result != VK_SUCCESS)
        return VulkanResult::Error(result, "Failed to get the instance layers");

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