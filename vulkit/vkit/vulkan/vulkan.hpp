#pragma once

#include "vkit/core/alias.hpp"
#include "tkit/utils/result.hpp"
#include "tkit/container/array.hpp"
#include "tkit/preprocessor/utils.hpp"
#include <vulkan/vulkan.h>
#include <functional>

#if !defined(VKIT_NO_DISCARD) && defined(TKIT_ENABLE_ASSERTS)
#    define VKIT_NO_DISCARD [[nodiscard]]
#else
#    define VKIT_NO_DISCARD
#endif

#ifdef VK_MAKE_API_VERSION
#    define VKIT_MAKE_VERSION(p_Variant, p_Major, p_Minor, p_Patch)                                                    \
        VK_MAKE_API_VERSION(p_Variant, p_Major, p_Minor, p_Patch)
#elif defined(VK_MAKE_VERSION)
#    define VKIT_MAKE_VERSION(p_Variant, p_Major, p_Minor, p_Patch) VK_MAKE_VERSION(p_Major, p_Minor, p_Patch)
#endif

#if defined(VK_API_VERSION_1_4) || defined(VK_VERSION_1_4)
#    define VKIT_API_VERSION_1_4 VKIT_MAKE_VERSION(0, 1, 4, 0)
#endif

#if defined(VK_API_VERSION_1_3) || defined(VK_VERSION_1_3)
#    define VKIT_API_VERSION_1_3 VKIT_MAKE_VERSION(0, 1, 3, 0)
#endif

#if defined(VK_API_VERSION_1_2) || defined(VK_VERSION_1_2)
#    define VKIT_API_VERSION_1_2 VKIT_MAKE_VERSION(0, 1, 2, 0)
#endif

#if defined(VK_API_VERSION_1_1) || defined(VK_VERSION_1_1)
#    define VKIT_API_VERSION_1_1 VKIT_MAKE_VERSION(0, 1, 1, 0)
#endif

#if defined(VK_API_VERSION_1_0) || defined(VK_VERSION_1_0)
#    define VKIT_API_VERSION_1_0 VKIT_MAKE_VERSION(0, 1, 0, 0)
#endif

#ifdef VK_API_VERSION_MAJOR
#    define VKIT_API_VERSION_MAJOR(p_Version) VK_API_VERSION_MAJOR(p_Version)
#elif defined(VK_VERSION_MAJOR)
#    define VKIT_API_VERSION_MAJOR(p_Version) VK_VERSION_MAJOR(p_Version)
#endif

#ifdef VK_API_VERSION_MINOR
#    define VKIT_API_VERSION_MINOR(p_Version) VK_API_VERSION_MINOR(p_Version)
#elif defined(VK_VERSION_MINOR)
#    define VKIT_API_VERSION_MINOR(p_Version) VK_VERSION_MINOR(p_Version)
#endif

#ifdef VK_API_VERSION_PATCH
#    define VKIT_API_VERSION_PATCH(p_Version) VK_API_VERSION_PATCH(p_Version)
#elif defined(VK_VERSION_PATCH)
#    define VKIT_API_VERSION_PATCH(p_Version) VK_VERSION_PATCH(p_Version)
#endif

#ifdef VK_API_VERSION_VARIANT
#    define VKIT_API_VERSION_VARIANT(p_Version) VK_API_VERSION_VARIANT(p_Version)
#elif defined(VK_VERSION_VARIANT)
#    define VKIT_API_VERSION_VARIANT(p_Version) VK_VERSION_VARIANT(p_Version)
#endif

#define VKIT_EXPAND_VERSION(p_Version)                                                                                 \
    VKIT_API_VERSION_MAJOR(p_Version), VKIT_API_VERSION_MINOR(p_Version), VKIT_API_VERSION_PATCH(p_Version)

#ifdef TKIT_ENABLE_DEBUG_LOGS
#    define VKIT_LOG_RESULT_DEBUG(p_Result)                                                                            \
        TKIT_LOG_DEBUG_IF(!VKit::IsSuccessful(p_Result), "[VULKIT][RESULT] {}", VKit::ResultToString(p_Result))
#    define VKIT_LOG_EXPRESSION_DEBUG(p_Expression)                                                                    \
        {                                                                                                              \
            const auto __vkit_result = p_Expression;                                                                   \
            TKIT_LOG_DEBUG_IF(!VKit::IsSuccessful(__vkit_result), "[VULKIT][RESULT] {}",                               \
                              VKit::ResultToString(__vkit_result));                                                    \
        }
#else
#    define VKIT_LOG_RESULT_DEBUG(p_Result) TKIT_UNUSED(p_Result)
#    define VKIT_LOG_EXPRESSION_DEBUG(p_Expression) p_Expression
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
#    define VKIT_LOG_RESULT_INFO(p_Result)                                                                             \
        TKIT_LOG_INFO_IF(!VKit::IsSuccessful(p_Result), "[VULKIT][RESULT] {}", VKit::ResultToString(p_Result))
#    define VKIT_LOG_EXPRESSION_INFO(p_Expression)                                                                     \
        {                                                                                                              \
            const auto __vkit_result = p_Expression;                                                                   \
            TKIT_LOG_INFO_IF(!VKit::IsSuccessful(__vkit_result), "[VULKIT][RESULT] {}",                                \
                             VKit::ResultToString(__vkit_result));                                                     \
        }
#else
#    define VKIT_LOG_RESULT_INFO(p_Result) TKIT_UNUSED(p_Result)
#    define VKIT_LOG_EXPRESSION_INFO(p_Expression) p_Expression
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define VKIT_LOG_RESULT_WARNING(p_Result)                                                                          \
        TKIT_LOG_WARNING_IF(!VKit::IsSuccessful(p_Result), "[VULKIT][RESULT] {}", VKit::ResultToString(p_Result))
#    define VKIT_LOG_EXPRESSION_WARNING(p_Expression)                                                                  \
        {                                                                                                              \
            const auto __vkit_result = p_Expression;                                                                   \
            TKIT_LOG_WARNING_IF(!VKit::IsSuccessful(__vkit_result), "[VULKIT][RESULT] {}",                             \
                                VKit::ResultToString(__vkit_result));                                                  \
        }
#else
#    define VKIT_LOG_RESULT_WARNING(p_Result) TKIT_UNUSED(p_Result)
#    define VKIT_LOG_EXPRESSION_WARNING(p_Expression) p_Expression
#endif

#ifdef TKIT_ENABLE_ERROR_LOGS
#    define VKIT_LOG_RESULT_ERROR(p_Result)                                                                            \
        TKIT_LOG_ERROR_IF(!VKit::IsSuccessful(p_Result), "[VULKIT][RESULT] {}", VKit::ResultToString(p_Result))
#    define VKIT_LOG_EXPRESSION_ERROR(p_Expression)                                                                    \
        {                                                                                                              \
            const auto __vkit_result = p_Expression;                                                                   \
            TKIT_LOG_ERROR_IF(!VKit::IsSuccessful(__vkit_result), "[VULKIT][RESULT] {}",                               \
                              VKit::ResultToString(__vkit_result));                                                    \
        }
#else
#    define VKIT_LOG_RESULT_ERROR(p_Result) TKIT_UNUSED(p_Result)
#    define VKIT_LOG_EXPRESSION_ERROR(p_Expression) p_Expression
#endif

#ifdef TKIT_ENABLE_ASSERTS
#    define VKIT_CHECK_RESULT(p_Result)                                                                                \
        TKIT_ASSERT(VKit::IsSuccessful(p_Result), "[VULKIT][RESULT] {}", VKit::ResultToString(p_Result))
#    define VKIT_CHECK_EXPRESSION(p_Expression)                                                                        \
        {                                                                                                              \
            const auto __vkit_result = p_Expression;                                                                   \
            TKIT_ASSERT(VKit::IsSuccessful(__vkit_result), "[VULKIT][RESULT] {}",                                      \
                        VKit::ResultToString(__vkit_result));                                                          \
        }
#else
#    define VKIT_CHECK_RESULT(p_Result) TKIT_UNUSED(p_Result)
#    define VKIT_CHECK_EXPRESSION(p_Expression) p_Expression
#endif

namespace VKit
{
enum ErrorCode : u8
{
    Error_VulkanError,
    Error_VulkanLibraryNotFound,
    Error_BadInput,
    Error_VersionMismatch,
    Error_NoSurfaceCapabilities,
    Error_RejectedDevice,
    Error_MissingQueue,
    Error_MissingExtension,
    Error_MissingFeature,
    Error_MissingLayer,
    Error_InsufficientMemory,
    Error_NoDeviceFound,
    Error_NoFormatSupported,
    Error_BadImageCount,
    Error_FileNotFound,
    Error_BadSynchronization,
    Error_Unknown,
    Error_Count
};

const char *VulkanResultToString(VkResult p_Result);
const char *ErrorCodeToString(ErrorCode p_Code);

template <typename T> bool IsSuccessful(const T &p_Result)
{
    using TT = std::remove_cvref_t<T>;
    if constexpr (std::same_as<TT, VkResult>)
        return p_Result == VK_SUCCESS;
    else
        return p_Result.IsOk();
}

template <typename T> auto ResultToString(const T &p_Result)
{
    using TT = std::remove_cvref_t<T>;
    if constexpr (std::same_as<TT, VkResult>)
        return VulkanResultToString(p_Result);
    else
    {
        TKIT_ASSERT(!p_Result, "[VULKIT][RESULT] Only unsuccessful results make sense to be stringified");
        return p_Result ? "Success" : p_Result.GetError().ToString();
    }
}
} // namespace VKit
namespace VKit::Detail
{
class Error
{
  public:
    Error() = default;
    Error(const ErrorCode p_Code, const std::string &p_Message) : m_FormattedMessage(p_Message), m_ErrorCode(p_Code)
    {
    }
    Error(const ErrorCode p_Code, const char *p_Message) : m_CheapMessage(p_Message), m_ErrorCode(p_Code)
    {
    }
    Error(const VkResult p_Result, const std::string &p_Message)
        : m_FormattedMessage(p_Message), m_ErrorCode(Error_VulkanError), m_VkResult(p_Result)
    {
    }
    Error(const VkResult p_Result, const char *p_Message)
        : m_CheapMessage(p_Message), m_ErrorCode(Error_VulkanError), m_VkResult(p_Result)
    {
    }
    Error(const ErrorCode p_Code) : m_ErrorCode(p_Code)
    {
    }
    Error(const VkResult p_Result) : m_ErrorCode(Error_VulkanError), m_VkResult(p_Result)
    {
    }

    std::string ToString() const;
    std::string GetMessage() const
    {
        return m_CheapMessage ? m_CheapMessage : m_FormattedMessage;
    }

    VkResult GetVulkanResult() const
    {
        return m_VkResult;
    }
    ErrorCode GetCode() const
    {
        return m_ErrorCode;
    }

    operator VkResult() const
    {
        return m_VkResult;
    }

    const char *m_CheapMessage = nullptr;
    std::string m_FormattedMessage{};
    ErrorCode m_ErrorCode = Error_Unknown;
    VkResult m_VkResult = VK_SUCCESS;
};
} // namespace VKit::Detail

namespace VKit
{
template <typename T = void> using Result = TKit::Result<T, Detail::Error>;

class DeletionQueue
{
  public:
    void Push(std::function<void()> &&p_Deleter);
    void Flush();

    template <typename VKitObject> void SubmitForDeletion(VKitObject p_Object)
    {
        Push([=]() mutable { p_Object.Destroy(); });
    }

  private:
    TKit::Array1024<std::function<void()>> m_Deleters;
};

} // namespace VKit
