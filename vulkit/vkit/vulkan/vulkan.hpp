#pragma once

#include "tkit/utils/result.hpp"
#include "tkit/container/array.hpp"
#include <vulkan/vulkan.h>
#include <functional>

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

#ifdef TKIT_ENABLE_DEBUG_LOGS
#    define VKIT_LOG_RESULT_DEBUG(p_Result)                                                                            \
        TKIT_LOG_DEBUG_IF(!VKit::IsSuccessful(p_Result), "[VULKIT] {}", VKit::ResultToString(p_Result))
#    define VKIT_LOG_EXPRESSION_DEBUG(p_Expression)                                                                    \
        {                                                                                                              \
            const auto __vkit_result = p_Expression;                                                                   \
            TKIT_LOG_DEBUG_IF(!VKit::IsSuccessful(__vkit_result), "[VULKIT] {}", VKit::ResultToString(__vkit_result)); \
        }
#else
#    define VKIT_LOG_RESULT_DEBUG(p_Result)
#    define VKIT_LOG_EXPRESSION_DEBUG(p_Expression) p_Expression
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
#    define VKIT_LOG_RESULT_INFO(p_Result)                                                                             \
        TKIT_LOG_INFO_IF(!VKit::IsSuccessful(p_Result), "[VULKIT] {}", VKit::ResultToString(p_Result))
#    define VKIT_LOG_EXPRESSION_INFO(p_Expression)                                                                     \
        {                                                                                                              \
            const auto __vkit_result = p_Expression;                                                                   \
            TKIT_LOG_INFO_IF(!VKit::IsSuccessful(__vkit_result), "[VULKIT] {}", VKit::ResultToString(__vkit_result));  \
        }
#else
#    define VKIT_LOG_RESULT_INFO(p_Result)
#    define VKIT_LOG_EXPRESSION_INFO(p_Expression) p_Expression
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define VKIT_LOG_RESULT_WARNING(p_Result)                                                                          \
        TKIT_LOG_WARNING_IF(!VKit::IsSuccessful(p_Result), "[VULKIT] {}", VKit::ResultToString(p_Result))
#    define VKIT_LOG_EXPRESSION_WARNING(p_Expression)                                                                  \
        {                                                                                                              \
            const auto __vkit_result = p_Expression;                                                                   \
            TKIT_LOG_WARNING_IF(!VKit::IsSuccessful(__vkit_result), "[VULKIT] {}",                                     \
                                VKit::ResultToString(__vkit_result));                                                  \
        }
#else
#    define VKIT_LOG_RESULT_WARNING(p_Result)
#    define VKIT_LOG_EXPRESSION_WARNING(p_Expression) p_Expression
#endif

#ifdef TKIT_ENABLE_ERROR_LOGS
#    define VKIT_LOG_RESULT_ERROR(p_Result)                                                                            \
        TKIT_LOG_ERROR_IF(!VKit::IsSuccessful(p_Result), "[VULKIT] {}", VKit::ResultToString(p_Result))
#    define VKIT_LOG_EXPRESSION_ERROR(p_Expression)                                                                    \
        {                                                                                                              \
            const auto __vkit_result = p_Expression;                                                                   \
            TKIT_LOG_ERROR_IF(!VKit::IsSuccessful(__vkit_result), "[VULKIT] {}", VKit::ResultToString(__vkit_result)); \
        }
#else
#    define VKIT_LOG_RESULT_ERROR(p_Result)
#    define VKIT_LOG_EXPRESSION_ERROR(p_Expression) p_Expression
#endif

#ifdef TKIT_ENABLE_ASSERTS
#    define VKIT_ASSERT_RESULT(p_Result)                                                                               \
        TKIT_ASSERT(VKit::IsSuccessful(p_Result), "[VULKIT] {}", VKit::ResultToString(p_Result))
#    define VKIT_ASSERT_EXPRESSION(p_Expression)                                                                       \
        {                                                                                                              \
            const auto __vkit_result = p_Expression;                                                                   \
            TKIT_ASSERT(VKit::IsSuccessful(__vkit_result), "[VULKIT] {}", VKit::ResultToString(__vkit_result));        \
        }
#else
#    define VKIT_ASSERT_RESULT(p_Result)
#    define VKIT_ASSERT_EXPRESSION(p_Expression) p_Expression
#endif

#define VKIT_FORMAT_ERROR(p_Result, ...) VKit::Detail::Error(p_Result, TKit::Format(__VA_ARGS__))

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
const char *VkResultToString(VkResult p_Result);

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
        return VkResultToString(p_Result);
    else
    {
        TKIT_ASSERT(!p_Result, "[VULKIT] Only unsuccessful results make sense to be stringified");
        return p_Result.GetError().ToString();
    }
}
} // namespace VKit
namespace VKit::Detail
{
struct Error
{
    Error() = default;
    Error(const VkResult p_Error, const std::string &p_Message) : ErrorCode(p_Error), FormattedMessage(p_Message)
    {
    }
    Error(const VkResult p_Error, const char *p_Message) : ErrorCode(p_Error), CheapMessage(p_Message)
    {
    }

    std::string ToString() const;
    std::string GetMessage() const
    {
        return CheapMessage ? CheapMessage : FormattedMessage;
    }

    operator VkResult() const
    {
        return ErrorCode;
    }

    VkResult ErrorCode{};
    const char *CheapMessage = nullptr;
    std::string FormattedMessage{};
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
