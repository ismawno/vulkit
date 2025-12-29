#pragma once

#include "vkit/core/alias.hpp"

namespace VKit
{
#ifndef VKIT_MAX_SHADER_SIZE
#    define VKIT_MAX_SHADER_SIZE 128 * 1024
#endif

constexpr u32 MaxShaderSize = VKIT_MAX_SHADER_SIZE;
}; // namespace VKit
