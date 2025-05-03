#include "vkit/core/pch.hpp"
#include "vkit/vulkan/vulkan.hpp"

namespace VKit
{
template <String MessageType>
ErrorInfo<MessageType>::ErrorInfo(VkResult p_Error, const MessageType &p_Message) noexcept
    : ErrorCode(p_Error), Message(p_Message)
{
}

template <String MessageType> ErrorInfo<MessageType>::operator VkResult() const noexcept
{
    return ErrorCode;
}

template class VKIT_API ErrorInfo<const char *>;
template class VKIT_API ErrorInfo<std::string>;

FormattedError ToFormatted(const Error &p_Error) noexcept
{
    return FormattedError{p_Error.ErrorCode, p_Error.Message};
}
} // namespace VKit