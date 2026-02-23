#pragma once

#include "vkit/core/alias.hpp"
#include "tkit/utils/result.hpp"
#include "tkit/container/tier_array.hpp"
#include "tkit/preprocessor/utils.hpp"
#include <vulkan/vulkan.h>
#include <functional>

#if !defined(VKIT_NO_DISCARD) && defined(TKIT_ENABLE_ASSERTS)
#    define VKIT_NO_DISCARD [[nodiscard]]
#else
#    define VKIT_NO_DISCARD
#endif

#ifdef VK_MAKE_API_VERSION
#    define VKIT_MAKE_VERSION(variant, major, minor, patch) VK_MAKE_API_VERSION(variant, major, minor, patch)
#elif defined(VK_MAKE_VERSION)
#    define VKIT_MAKE_VERSION(variant, major, minor, patch) VK_MAKE_VERSION(major, minor, patch)
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
#    define VKIT_API_VERSION_MAJOR(version) VK_API_VERSION_MAJOR(version)
#elif defined(VK_VERSION_MAJOR)
#    define VKIT_API_VERSION_MAJOR(version) VK_VERSION_MAJOR(version)
#endif

#ifdef VK_API_VERSION_MINOR
#    define VKIT_API_VERSION_MINOR(version) VK_API_VERSION_MINOR(version)
#elif defined(VK_VERSION_MINOR)
#    define VKIT_API_VERSION_MINOR(version) VK_VERSION_MINOR(version)
#endif

#ifdef VK_API_VERSION_PATCH
#    define VKIT_API_VERSION_PATCH(version) VK_API_VERSION_PATCH(version)
#elif defined(VK_VERSION_PATCH)
#    define VKIT_API_VERSION_PATCH(version) VK_VERSION_PATCH(version)
#endif

#ifdef VK_API_VERSION_VARIANT
#    define VKIT_API_VERSION_VARIANT(version) VK_API_VERSION_VARIANT(version)
#elif defined(VK_VERSION_VARIANT)
#    define VKIT_API_VERSION_VARIANT(version) VK_VERSION_VARIANT(version)
#endif

#define VKIT_EXPAND_VERSION(version)                                                                                   \
    VKIT_API_VERSION_MAJOR(version), VKIT_API_VERSION_MINOR(version), VKIT_API_VERSION_PATCH(version)

#ifdef TKIT_ENABLE_DEBUG_LOGS
#    define VKIT_LOG_RESULT_DEBUG(result)                                                                              \
        TKIT_LOG_DEBUG_IF(!VKit::IsSuccessful(result), "{}", VKit::ResultToString(result))
#    define VKIT_LOG_EXPRESSION_DEBUG(expression)                                                                      \
        {                                                                                                              \
            const auto __vkit_result = expression;                                                                     \
            TKIT_LOG_DEBUG_IF(!VKit::IsSuccessful(__vkit_result), "{}", VKit::ResultToString(__vkit_result));          \
        }
#else
#    define VKIT_LOG_RESULT_DEBUG(result) TKIT_UNUSED(result)
#    define VKIT_LOG_EXPRESSION_DEBUG(expression) expression
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
#    define VKIT_LOG_RESULT_INFO(result)                                                                               \
        TKIT_LOG_INFO_IF(!VKit::IsSuccessful(result), "{}", VKit::ResultToString(result))
#    define VKIT_LOG_EXPRESSION_INFO(expression)                                                                       \
        {                                                                                                              \
            const auto __vkit_result = expression;                                                                     \
            TKIT_LOG_INFO_IF(!VKit::IsSuccessful(__vkit_result), "{}", VKit::ResultToString(__vkit_result));           \
        }
#else
#    define VKIT_LOG_RESULT_INFO(result) TKIT_UNUSED(result)
#    define VKIT_LOG_EXPRESSION_INFO(expression) expression
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define VKIT_LOG_RESULT_WARNING(result)                                                                            \
        TKIT_LOG_WARNING_IF(!VKit::IsSuccessful(result), "{}", VKit::ResultToString(result))
#    define VKIT_LOG_EXPRESSION_WARNING(expression)                                                                    \
        {                                                                                                              \
            const auto __vkit_result = expression;                                                                     \
            TKIT_LOG_WARNING_IF(!VKit::IsSuccessful(__vkit_result), "{}", VKit::ResultToString(__vkit_result));        \
        }
#else
#    define VKIT_LOG_RESULT_WARNING(result) TKIT_UNUSED(result)
#    define VKIT_LOG_EXPRESSION_WARNING(expression) expression
#endif

#ifdef TKIT_ENABLE_ERROR_LOGS
#    define VKIT_LOG_RESULT_ERROR(result)                                                                              \
        TKIT_LOG_ERROR_IF(!VKit::IsSuccessful(result), "{}", VKit::ResultToString(result))
#    define VKIT_LOG_EXPRESSION_ERROR(expression)                                                                      \
        {                                                                                                              \
            const auto __vkit_result = expression;                                                                     \
            TKIT_LOG_ERROR_IF(!VKit::IsSuccessful(__vkit_result), "{}", VKit::ResultToString(__vkit_result));          \
        }
#else
#    define VKIT_LOG_RESULT_ERROR(result) TKIT_UNUSED(result)
#    define VKIT_LOG_EXPRESSION_ERROR(expression) expression
#endif

#ifdef TKIT_ENABLE_ASSERTS
#    define VKIT_CHECK_RESULT(result) TKIT_ASSERT(VKit::IsSuccessful(result), "{}", VKit::ResultToString(result))
#else
#    define VKIT_CHECK_RESULT(result) TKIT_UNUSED(result)
#endif

#define VKIT_CHECK_EXPRESSION(expression) VKit::CheckExpression(expression)

#define VKIT_RETURN_ON_ERROR(vkresult, rtype, ...)                                                                     \
    if ((vkresult) != VK_SUCCESS)                                                                                      \
    {                                                                                                                  \
        __VA_ARGS__;                                                                                                   \
        return rtype::Error((vkresult));                                                                               \
    }

#define VKIT_RETURN_IF_FAILED(expression, rtype, ...)                                                                  \
    if (const VkResult __vkit_result = (expression); __vkit_result != VK_SUCCESS)                                      \
    {                                                                                                                  \
        __VA_ARGS__;                                                                                                   \
        return rtype::Error(__vkit_result);                                                                            \
    }

#define VKIT_RETURN_ON_ERROR_WITH_MESSAGE(vkresult, rtype, msg, ...)                                                   \
    if ((vkresult) != VK_SUCCESS)                                                                                      \
    {                                                                                                                  \
        __VA_ARGS__;                                                                                                   \
        return rtype::Error((vkresult), (msg));                                                                        \
    }

#define VKIT_RETURN_IF_FAILED_WITH_MESSAGE(expression, rtype, msg, ...)                                                \
    if (const VkResult __vkit_result = (expression); __vkit_result != VK_SUCCESS)                                      \
    {                                                                                                                  \
        __VA_ARGS__;                                                                                                   \
        return rtype::Error(__vkit_result, (msg));                                                                     \
    }

#define VKIT_RETURN_ON_ERROR_FORMATTED(vkresult, rtype, ...)                                                           \
    if ((vkresult) != VK_SUCCESS)                                                                                      \
    return rtype::Error((vkresult), TKit::Format(__VA_ARGS__))

#define VKIT_RETURN_IF_FAILED_FORMATTED(expression, rtype, ...)                                                        \
    if (const VkResult __vkit_result = (expression); __vkit_result != VK_SUCCESS)                                      \
    return rtype::Error(__vkit_result, TKit::Format(__VA_ARGS__))

#ifdef VK_EXT_debug_utils
#    define VKIT_SET_DEBUG_NAME(handle, objType)                                                                       \
        VKIT_NO_DISCARD Result<> SetName(const char *name)                                                             \
        {                                                                                                              \
            return m_Device.SetObjectName(handle, objType, name);                                                      \
        }
#else
#    define VKIT_SET_DEBUG_NAME(...)
#endif

namespace VKit
{
enum ErrorCode : u8
{
    Error_VulkanError,
    Error_VulkanLibraryNotFound,
    Error_InitializationFailed,
    Error_LoadFailed,
    Error_BadInput,
    Error_VersionMismatch,
    Error_NoSurfaceCapabilities,
    Error_RejectedWindow,
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
    Error_EntryPointNotFound,
    Error_ShaderCompilationFailed,
    Error_BadSynchronization,
    Error_Unknown,
    Error_Count
};

template <typename T, typename E> T CheckExpression(TKit::Result<T, E> &&result)
{
    TKIT_ASSERT(result, "{}", result.GetError().ToString());
    if constexpr (!std::same_as<T, void>)
        return result.GetValue();
}

#ifdef TKIT_ENABLE_ASSERTS
void CheckExpression(VkResult result);
#else
inline void CheckExpression(const VkResult)
{
}
#endif

const char *VulkanResultToString(VkResult result);
const char *ErrorCodeToString(ErrorCode code);

template <typename T> bool IsSuccessful(const T &result)
{
    using TT = std::remove_cvref_t<T>;
    if constexpr (std::same_as<TT, VkResult>)
        return result == VK_SUCCESS;
    else
        return result.IsOk();
}

template <typename T> auto ResultToString(const T &result)
{
    using TT = std::remove_cvref_t<T>;
    if constexpr (std::same_as<TT, VkResult>)
        return VulkanResultToString(result);
    else
    {
        TKIT_ASSERT(!result, "[VULKIT][RESULT] Only unsuccessful results make sense to be stringified");
        return result ? "Success" : result.GetError().ToString();
    }
}
} // namespace VKit
namespace VKit::Detail
{
class Error
{
  public:
    Error() = default;
    Error(const ErrorCode code, const std::string &message) : m_FormattedMessage(message), m_ErrorCode(code)
    {
    }
    Error(const ErrorCode code, const char *message) : m_CheapMessage(message), m_ErrorCode(code)
    {
    }
    Error(const VkResult result, const std::string &message)
        : m_FormattedMessage(message), m_ErrorCode(Error_VulkanError), m_VkResult(result)
    {
    }
    Error(const VkResult result, const char *message)
        : m_CheapMessage(message), m_ErrorCode(Error_VulkanError), m_VkResult(result)
    {
    }
    Error(const ErrorCode code) : m_ErrorCode(code)
    {
    }
    Error(const VkResult result) : m_ErrorCode(Error_VulkanError), m_VkResult(result)
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
    TKIT_NON_COPYABLE(DeletionQueue)
  public:
    DeletionQueue() = default;
    ~DeletionQueue()
    {
        Flush();
    }

    void Push(std::function<void()> &&deleter);
    void Flush();
    void Dismiss();

    template <typename VKitObject> void SubmitForDeletion(VKitObject object)
    {
        Push([=]() mutable { object.Destroy(); });
    }

  private:
    TKit::TierArray<std::function<void()>> m_Deleters{};
};

} // namespace VKit
