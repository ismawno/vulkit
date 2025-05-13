#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "tkit/utils/result.hpp"
#include <vulkan/vulkan.h>

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

#ifdef TKIT_ENABLE_ASSERTS
#    define VKIT_ASSERT_RESULT(result)                                                                                 \
        TKIT_ASSERT(result, "[VULKIT] VkResult: '{}' - Message: '{}'",                                                 \
                    VKit::VkResultToString(result.GetError().Result), result.GetError().Message)
#else
#    define VKIT_ASSERT_RESULT(result)
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
#    define VKIT_LOG_RESULT(result)                                                                                    \
        TKIT_LOG_INFO_IF(!result, "[VULKIT] VkResult: '{}' - Message: '{}'",                                           \
                         VKit::VkResultToString(result.GetError().Result), result.GetError().Message)
#else
#    define VKIT_LOG_RESULT(result)
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define VKIT_WARN_RESULT(result)                                                                                   \
        TKIT_LOG_WARNING_IF(!result, "[VULKIT] VkResult: '{}' - Message: '{}'",                                        \
                            VKit::VkResultToString(result.GetError().Result), result.GetError().Message)
#else
#    define VKIT_WARN_RESULT(result)
#endif

#define VKIT_FORMAT_ERROR(p_Result, ...) VKit::ErrorInfo<std::string>(p_Result, TKIT_FORMAT(__VA_ARGS__))

#ifdef TKIT_ENABLE_ASSERTS
#    define VKIT_CHECK_GLOBAL_FUNCTION_OR_RETURN(p_Func, p_Result)                                                     \
        if (!VKit::Vulkan::p_Func)                                                                                     \
        return p_Result::Error(VK_ERROR_INCOMPATIBLE_DRIVER, "Failed to load Vulkan function: " #p_Func)

#    define VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Table, p_Func, p_Result)                                             \
        if (!p_Table->p_Func)                                                                                          \
        return p_Result::Error(VK_ERROR_INCOMPATIBLE_DRIVER, "Failed to load Vulkan function: " #p_Func)
#else
#    define VKIT_CHECK_GLOBAL_FUNCTION_OR_RETURN(p_Func, p_Result)
#    define VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Table, p_Func, p_Result)
#endif

namespace VKit
{
template <typename T>
concept String = std::is_same_v<T, const char *> || std::is_same_v<T, std::string>;

/**
 * @brief Represents an error in a Vulkan operation, including error type and message.
 *
 * This class encapsulates a non-success Vulkan result code (`VkResult`) and an optional message to provide
 * more context about the operation's outcome.
 *
 * Using the default constructor will create an uninitialized `ErrorInfo`. Make sure to instantiate it with the `Error`
 * function before using it.
 *
 * @tparam MessageType The type of the message, either `const char*` or `std::string`. The former is a cheap version,
 * when the message is a static string. The latter is used when explicit error message information can be provided based
 * on user input.
 */
template <String MessageType> class ErrorInfo
{
  public:
    ErrorInfo() noexcept = default;
    /**
     * @brief Creates an error result with the given Vulkan result code and message.
     *
     * @param p_Error The Vulkan result code indicating the error.
     * @param p_Message A descriptive message providing details about the error.
     * @return A `ErrorInfo` instance representing the error.
     */
    ErrorInfo(VkResult p_Error, const MessageType &p_Message) noexcept;

    operator VkResult() const noexcept;

    VkResult ErrorCode{};
    MessageType Message{};
};

using Error = ErrorInfo<const char *>;
using FormattedError = ErrorInfo<std::string>;

template <typename T = void> using Result = TKit::Result<T, Error>;
template <typename T = void> using FormattedResult = TKit::Result<T, FormattedError>;

VKIT_API FormattedError ToFormatted(const Error &p_Result) noexcept;
template <typename T> FormattedResult<T> ToFormatted(const Result<T> &p_Result) noexcept
{
    return p_Result ? FormattedResult<T>::Ok(p_Result.GetValue())
                    : FormattedResult<T>::Error(p_Result.GetError().Result, p_Result.GetError().Message);
}
} // namespace VKit
