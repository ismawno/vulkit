#include "vkit/core/pch.hpp"
#include "vkit/vulkan/vulkan.hpp"

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

template class VKIT_API VulkanResultInfo<const char *>;
template class VKIT_API VulkanResultInfo<std::string>;

VulkanFormattedResult ToFormatted(const VulkanResult &p_Result) noexcept
{
    return VulkanFormattedResult{p_Result.Result, p_Result.Message};
}
} // namespace VKit