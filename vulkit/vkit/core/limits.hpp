#pragma once

#include "vkit/core/alias.hpp"

namespace VKit
{
#ifndef VKIT_MAX_SHADER_SIZE
#    define VKIT_MAX_SHADER_SIZE 16 * 1024
#endif

#ifndef VKIT_MAX_QUEUE_COUNT
#    define VKIT_MAX_QUEUE_COUNT 32
#endif

#ifndef VKIT_MAX_QUEUE_SUBMISSIONS
#    define VKIT_MAX_QUEUE_SUBMISSIONS 16
#endif

constexpr u32 MaxShaderSize = VKIT_MAX_SHADER_SIZE;
constexpr u32 MaxQueueCount = VKIT_MAX_QUEUE_COUNT;
constexpr u32 MaxQueueSubmissions = VKIT_MAX_QUEUE_SUBMISSIONS;
}; // namespace VKit
