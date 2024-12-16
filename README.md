# Vulkit

Vulkit is a C++ utility library I have designed to simplify and streamline working with the Vulkan API. Its primary focus is to accelerate the headache that is the Vulkan initialization process. In addition, Vulkit provides high-level abstractions for essential components in graphics programming, such as buffers, descriptors, swap chains, command pools, and more, making it a versatile tool for Vulkan-based development.

This library was started as I realized how many interesting projects the Vulkan API had unlocked for me, especially regarding graphics programming and GPGPU. I originally had a somewhat hardcoded setup in [Onyx](https://github.com/ismawno/onyx) (an application framework library I am still developing), which I didn’t really like because of its lack of flexibility. So, taking inspiration from the great [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap) library, I decided to implement my own.

The reason I decided against using an existing Vulkan library to handle all of this for me is because I am actually very interested in the initialization and low-level manipulation of the library. Plus, I wanted to have control over the design choices and coding style.

## Features

Vulkit focuses on Vulkan API abstractions, which can be summed up as follows:
- Significant speedup of the Vulkan API initialization process
- Significant speedup of instance, physical device, and logical device creation
- Useful abstractions of essential components of the Vulkan API, such as buffers, command pools, swap chains, render passes, etc.

### API Initialization

In Vulkit, Vulkan initialization consists of 4 steps:

1. Instance extensions and layers support querying
2. Instance creation
3. Physical device selection
4. Logical device creation

All of these steps are pretty straightforward, especially the first one, consisting of a single function call:

```cpp
const auto sysres = VKit::System::Initialize();
if (!sysres) 
{
    // Handle error
}
```

Almost all Vulkit API calls will return a result object. In the case of otherwise `void` functions, an informative result object will be returned containing no return value, of type `VulkanResult`, which may or may not contain an error. It supports a trivial `bool` implicit conversion. When the function is expected to return a value, a `Result` object will be provided, allowing you to access the underlying value with the `GetValue()` method, assuming no error occurred.

Vulkit provides an easy and convenient way of checking these results using the logging capabilities of the [Toolkit](https://github.com/ismawno/toolkit) through the following macros: `VKIT_ASSERT_VULKAN_RESULT(result)` for `VulkanResult` and `VKIT_ASSERT_RESULT(result)` for `Result`.

Moving on:

```cpp
const auto sysres = VKit::System::Initialize();
VKIT_ASSERT_VULKAN_RESULT(result);
```

Creating an instance is straightforward as well with the `VKit::Instance::Builder` object:

```cpp
const char *extensions[] = {/* my extensions */};
const auto result = VKit::Instance::Builder()
                        .SetApplicationName("Onyx")
                        .RequireApiVersion(1, 1, 0)
                        .RequireExtensions(extensions)
                        .RequestExtension("VK_extra_extension")
                        .RequestValidationLayers()
                        .Build();
VKIT_ASSERT_RESULT(result);
```

The result object returned encapsulates a `VKit::Instance` object, which contains a `VkInstance` handle. Almost all Vulkit objects, including `VKit::Instance`, are simple, trivially copyable POD classes. This design means resource cleanup is entirely the user's responsibility. However, Vulkit minimizes the effort required by providing `Destroy` and `SubmitForDeletion` methods for all classes that manage Vulkan resources. Additionally, a `DeletionQueue` class is available for use with the `SubmitForDeletion` method to streamline cleanup.

You can create a headless Vulkan instance by calling `SetHeadless(true)` on the builder, which will exclude the necessary extensions for presentation.

Physical and logical device creation follows a very similar pattern:


```cpp
const VkSurfaceKHR surface = /* get surface */;
const auto physres =
    VKit::PhysicalDevice::Selector(&instance)
        .SetSurface(surface)
        .PreferType(VKit::PhysicalDevice::Discrete)
        .AddFlags(VKit::PhysicalDevice::Selector::Flag_AnyType | VKit::PhysicalDevice::Selector::Flag_PortabilitySubset |
                    VKit::PhysicalDevice::Selector::Flag_RequireGraphicsQueue)
        .Select();
VKIT_ASSERT_RESULT(physres);
const VKit::PhysicalDevice &phys = physres.GetValue();

const auto devres = VKit::LogicalDevice::Create(s_Instance, phys);
VKIT_ASSERT_RESULT(devres);
```

Note that if the instance is not headless, a surface must be provided to select an appropriate physical device.

There are many more options available for both the [instance](https://github.com/ismawno/vulkit/blob/main/vulkit/vkit/backend/instance.hpp) and [physical device](https://github.com/ismawno/vulkit/blob/main/vulkit/vkit/backend/physical_device.hpp) creation, which can be explored by inspecting the corresponding class definitions.

### Other Abstractions

Vulkit provides a variety of abstractions. Here is the full list:

- [CommandPool](https://github.com/ismawno/vulkit/blob/main/vulkit/vkit/backend/command_pool.hpp): Simplifies the allocation and deallocation of command buffers, reducing verbosity.
- [RenderPass](https://github.com/ismawno/vulkit/blob/main/vulkit/vkit/rendering/render_pass.hpp): Simplifies the creation and management of Vulkan render passes, which can be complex due to the need to configure attachments, subpasses, and dependencies. Vulkit uses a flexible builder pattern to define render passes and provides utilities for managing associated resources like frame buffers and image views.
- [SwapChain](https://github.com/ismawno/vulkit/blob/main/vulkit/vkit/rendering/swap_chain.hpp): Streamlines the process of creating a swap chain, which can otherwise be cumbersome due to the variety of formats and parameters. Vulkit uses the same builder pattern as instance and device handles. It also supports creating additional resources that typically need to be recreated when the window is resized.
- [Buffers](https://github.com/ismawno/vulkit/tree/main/vulkit/vkit/buffer): Provides both low-level and high-level abstractions for various buffer usages (device-local and host-visible), as well as utility functions for creating vertex, index, uniform, and storage buffers.
- [Descriptors](https://github.com/ismawno/vulkit/tree/main/vulkit/vkit/descriptors): Includes three abstractions for managing descriptor pools, descriptor set layouts, and descriptor writes.
- [Pipelines and Shaders](https://github.com/ismawno/vulkit/tree/main/vulkit/vkit/pipeline): Offers abstractions for both graphics and compute pipelines. Additionally, a Shader class is provided, capable of compiling GLSL shaders into SPIR-V format by invoking the `glslc` compiler.

## Dependencies and Third-Party Libraries

Vulkit relies on the following dependencies:

- [Toolkit](https://github.com/ismawno/toolkit): A utility library I developed.
- [Vulkan Memory Allocator (VMA)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator): Simplifies Vulkan memory management.
- [Vulkan Loader](https://github.com/KhronosGroup/Vulkan-Loader): Provides the Vulkan API.

## Building

The build process is straightforward. Create a `build` folder, navigate into it, and run `cmake ..`. All available Vulkit options will be displayed.

Compile the project with your preferred editor or IDE, and run the tests to ensure everything works as expected. If the tests pass, you're good to go!

## Additional Notes

Tests are pretty mediocre, there are almost none.