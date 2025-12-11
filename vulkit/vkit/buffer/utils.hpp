#include <vulkan/vulkan.h>
namespace VKit
{
// for the sake of defaults
struct BufferCopy
{
    VkDeviceSize Size = VK_WHOLE_SIZE;
    VkDeviceSize SrcOffset = 0;
    VkDeviceSize DstOffset = 0;
};
} // namespace VKit
