#include "vkit/core/pch.hpp"
#include "vkit/resource/sampler.hpp"

namespace VKit
{
Sampler::Builder::Builder(const ProxyDevice &device) : m_Device(device)
{
    m_Info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    m_Info.magFilter = VK_FILTER_LINEAR;
    m_Info.minFilter = VK_FILTER_LINEAR;
    m_Info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    m_Info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    m_Info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    m_Info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    m_Info.mipLodBias = 0.0f;
    m_Info.anisotropyEnable = VK_FALSE;
    m_Info.maxAnisotropy = 1.0f;
    m_Info.compareEnable = VK_FALSE;
    m_Info.compareOp = VK_COMPARE_OP_ALWAYS;
    m_Info.minLod = 0.0f;
    m_Info.maxLod = VK_LOD_CLAMP_NONE;
    m_Info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    m_Info.unnormalizedCoordinates = VK_FALSE;
}

Result<Sampler> Sampler::Builder::Build() const
{
    VkSampler sampler;
    VKIT_RETURN_IF_FAILED(m_Device.Table->CreateSampler(m_Device, &m_Info, m_Device.AllocationCallbacks, &sampler),
                          Result<Sampler>);

    return Result<Sampler>::Ok(m_Device, sampler);
}

Sampler::Builder &Sampler::Builder::SetFilters(const VkFilter mag, const VkFilter min)
{
    m_Info.magFilter = mag;
    m_Info.minFilter = min;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetMagFilter(const VkFilter mag)
{
    m_Info.magFilter = mag;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetMinFilter(const VkFilter min)
{
    m_Info.minFilter = min;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetMipmapMode(const VkSamplerMipmapMode mode)
{
    m_Info.mipmapMode = mode;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetAddressModes(const VkSamplerAddressMode mode)
{
    m_Info.addressModeU = mode;
    m_Info.addressModeV = mode;
    m_Info.addressModeW = mode;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetAddressModes(const VkSamplerAddressMode u, const VkSamplerAddressMode v,
                                                    const VkSamplerAddressMode w)
{
    m_Info.addressModeU = u;
    m_Info.addressModeV = v;
    m_Info.addressModeW = w;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetAddressModeU(const VkSamplerAddressMode u)
{
    m_Info.addressModeU = u;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetAddressModeV(const VkSamplerAddressMode v)
{
    m_Info.addressModeV = v;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetAddressModeW(const VkSamplerAddressMode w)
{
    m_Info.addressModeW = w;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetMipLodBias(const f32 bias)
{
    m_Info.mipLodBias = bias;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetLodRange(const f32 min, const f32 max)
{
    m_Info.minLod = min;
    m_Info.maxLod = max;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetMinLod(const f32 min)
{
    m_Info.minLod = min;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetMaxLod(const f32 max)
{
    m_Info.maxLod = max;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetAnisotropy(const f32 maxAnisotropy)
{
    m_Info.anisotropyEnable = VK_TRUE;
    m_Info.maxAnisotropy = maxAnisotropy;
    return *this;
}

Sampler::Builder &Sampler::Builder::DisableAnisotropy()
{
    m_Info.anisotropyEnable = VK_FALSE;
    m_Info.maxAnisotropy = 1.0f;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetCompareOp(const VkCompareOp op)
{
    m_Info.compareEnable = VK_TRUE;
    m_Info.compareOp = op;
    return *this;
}

Sampler::Builder &Sampler::Builder::DisableCompare()
{
    m_Info.compareEnable = VK_FALSE;
    m_Info.compareOp = VK_COMPARE_OP_ALWAYS;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetBorderColor(const VkBorderColor color)
{
    m_Info.borderColor = color;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetUnnormalizedCoordinates(const bool unnormalized)
{
    m_Info.unnormalizedCoordinates = unnormalized ? VK_TRUE : VK_FALSE;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetFlags(const VkSamplerCreateFlags flags)
{
    m_Info.flags = flags;
    return *this;
}

Sampler::Builder &Sampler::Builder::AddFlags(const VkSamplerCreateFlags flags)
{
    m_Info.flags |= flags;
    return *this;
}

Sampler::Builder &Sampler::Builder::SetNext(const void *next)
{
    m_Info.pNext = next;
    return *this;
}
} // namespace VKit
