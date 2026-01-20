#pragma once

#include "vkit/vulkan/vulkan.hpp"
#include "tkit/memory/arena_allocator.hpp"
#include "tkit/memory/stack_allocator.hpp"
#include "tkit/memory/tier_allocator.hpp"

namespace VKit
{
using Flags = u8;
struct Allocation
{
    TKit::ArenaAllocator *Arena = nullptr;
    TKit::StackAllocator *Stack = nullptr;
    TKit::TierAllocator *Tier = nullptr;
};
struct Specs
{
    const char *LoaderPath = nullptr;
    Allocation Allocators{};
};
} // namespace VKit

namespace VKit::Core
{
Result<> Initialize(const Specs &p_Specs = {});
void Terminate();

bool IsExtensionSupported(const char *p_Name);
bool IsLayerSupported(const char *p_Name);

const VkExtensionProperties *GetExtensionByName(const char *p_Name);
const VkLayerProperties *GetLayerByName(const char *p_Name);

const VkExtensionProperties &GetExtensionByIndex(u32 p_Index);
const VkLayerProperties &GetLayerByIndex(u32 p_Index);

u32 GetExtensionCount();
u32 GetLayerCount();

} // namespace VKit::Core
