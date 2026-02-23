#pragma once

#include "vkit/device/proxy_device.hpp"

namespace VKit
{
class Sampler
{
  public:
    class Builder
    {
      public:
        Builder(const ProxyDevice &device);
        VKIT_NO_DISCARD Result<Sampler> Build() const;

        Builder &SetFilters(VkFilter mag, VkFilter min);
        Builder &SetMagFilter(VkFilter mag);
        Builder &SetMinFilter(VkFilter min);

        Builder &SetMipmapMode(VkSamplerMipmapMode mode);

        Builder &SetAddressModes(VkSamplerAddressMode mode);
        Builder &SetAddressModes(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w);
        Builder &SetAddressModeU(VkSamplerAddressMode u);
        Builder &SetAddressModeV(VkSamplerAddressMode v);
        Builder &SetAddressModeW(VkSamplerAddressMode w);

        Builder &SetMipLodBias(f32 bias);
        Builder &SetLodRange(f32 min, f32 max);
        Builder &SetMinLod(f32 min);
        Builder &SetMaxLod(f32 max);

        Builder &SetAnisotropy(f32 maxAnisotropy);
        Builder &DisableAnisotropy();

        Builder &SetCompareOp(VkCompareOp op);
        Builder &DisableCompare();

        Builder &SetBorderColor(VkBorderColor color);
        Builder &SetUnnormalizedCoordinates(bool unnormalized);

        Builder &SetFlags(VkSamplerCreateFlags flags);
        Builder &AddFlags(VkSamplerCreateFlags flags);
        Builder &SetNext(const void *next);

      private:
        ProxyDevice m_Device;
        VkSamplerCreateInfo m_Info{};
    };
    Sampler() = default;

    Sampler(const ProxyDevice &device, const VkSampler sampler) : m_Device(device), m_Sampler(sampler)
    {
    }

    void Destroy();

    VKIT_SET_DEBUG_NAME(m_Sampler, VK_OBJECT_TYPE_SAMPLER)
    const ProxyDevice &GetDevice()
    {
        return m_Device;
    }
    VkSampler GetHandle() const
    {
        return m_Sampler;
    }

    operator VkSampler() const
    {
        return m_Sampler;
    }
    operator bool() const
    {
        return m_Sampler != VK_NULL_HANDLE;
    }

  private:
    ProxyDevice m_Device{};
    VkSampler m_Sampler = VK_NULL_HANDLE;
};
} // namespace VKit
