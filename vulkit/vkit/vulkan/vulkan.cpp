#include "tkit/utils/logging.hpp"
#include "vkit/core/pch.hpp"
#include "vkit/vulkan/vulkan.hpp"
#include "vkit/core/alias.hpp"

#include <functional>

namespace VKit
{

template <String MessageType> std::string ErrorInfo<MessageType>::ToString() const
{
    return TKIT_FORMAT("VkResult: '{}' - Message: '{}'", VkResultToString(ErrorCode), Message);
}

template class VKIT_API ErrorInfo<const char *>;
template class VKIT_API ErrorInfo<std::string>;

void DeletionQueue::Push(std::function<void()> &&p_Deleter)
{
    m_Deleters.Append(std::move(p_Deleter));
}
void DeletionQueue::Flush()
{
    for (u32 i = m_Deleters.GetSize(); i > 0; --i)
        m_Deleters[i - 1]();
    m_Deleters.Clear();
}

FormattedError ToFormatted(const Error &p_Error)
{
    return FormattedError{p_Error.ErrorCode, p_Error.Message};
}
} // namespace VKit
