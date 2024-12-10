# Vulkit

Vulkit is a C++ utility library I have designed to simplify and streamline working with the Vulkan API. Its primary focus is to accelerate the headache that is the Vulkan initialization process. In addition, Vulkit provides high-level abstractions for essential components in graphics programming, such as buffers, descriptors, swap chains, command pools, and more, making it a versatile tool for Vulkan-based development.

This library was started as I realized how many interesting projects the Vulkan API had unlocked for me, specially regarding graphics programming and GPGPU. I originally had a somewhat hardcoded setup in [Onyx](https://github.com/ismawno/onyx) (an application framework library I am still developing), which I didnt really like because of its little flexibility. So, taking inspiration from the great [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap) library, I decided to implement my own.

The reason I decided against using an existing Vulkan library to handle all of this for me is because I am actually very interested in the initialization and low level manipulation of the library. Plus, I wanted to have control over the design choices and coding style.

## Features

Vulkit focuses on Vulkan API abstractions, which can be summed up as follows:
- Significant speed up of the Vulkan API initialization process
- Significant speed up of instance, physical device and logical device creation
- Useful abstractions of essential components of the Vulkan API, such as buffers, command pools, swap chains, etcetera.

### API Initialization

In Vulkit, Vulkan initialization consists of 4 steps:

1. Instance extensions and layers support querying
2. Instance creation
3. Physical device selection
4. Logical device creation

All of these steps are pretty straightforward, specially the first one, consisting on a single function call:

```cpp
const auto sysres = VKit::System::Initialize();
if (!sysres) 
{
    // Handle error
}
```

Almost all Vulkit API calls will return a result object. In case of otherwise `void` functions, an informative result object will be returned containing no return value of type `VulkanResult`, which may or may not contain an error. It has a trivial `bool` implicit conversion. When the function is supposed to have a value, a `Result` object will be returned, and you may access the underlying value with the `GetValue()` method, assuming no error was raised.

Vulkit provides an easy and convenient way of checking these results using the logging capabilities of the [Toolkit](https://github.com/ismawno/toolkit) with the following macros: `VKIT_ASSERT_VULKAN_RESULT(result)` for `VulkanResult` and `VKIT_ASSERT_RESULT(result)` for `Result`.

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

The result object returned encapsulates a `VKit::Instance` object, which in turn contains a `VkInstance` handle. Almost all Vulkit objects, including `VKit::Instance`, are mostly simple, trivially copyable POD classes. This means resource cleanup is entirely up to the user. Vulkit tries to keep this process as painless as possible however, providing `Destroy` and `SubmitForDeletion` methods for all classes that hold a managed Vulkan resource. A `DeletionQueue` class is also provided to use in conjunction to the latter destruction method.

You can create a headless vulkan instance by calling `SetHeadless(true)` on the builder, which will exlcude the necessary extensions for presentation.

Physical and logical device creation follow a very similar pattern:

```cpp
const VkSurfaceKHR surface = /* get surface */;
const auto physres =
    VKit::PhysicalDevice::Selector(&instance)
        .SetSurface(surface)
        .PreferType(VKit::PhysicalDevice::Discrete)
        .AddFlags(VKit::PhysicalDeviceSelectorFlags_AnyType | VKit::PhysicalDeviceSelectorFlags_PortabilitySubset |
                    VKit::PhysicalDeviceSelectorFlags_RequireGraphicsQueue)
        .Select();
VKIT_ASSERT_RESULT(physres);
const VKit::PhysicalDevice &phys = physres.GetValue();

const auto devres = VKit::LogicalDevice::Create(s_Instance, phys);
VKIT_ASSERT_RESULT(devres);
```

Note that if the instance is not headless, a surface must be provided to be able to choose an appropiate physical device.

There are many more options available for both the [instance](https://github.com/ismawno/vulkit/blob/main/vulkit/vkit/backend/instance.hpp) and [physical device](https://github.com/ismawno/vulkit/blob/main/vulkit/vkit/backend/physical_device.hpp) creation which can be checked out by inspecting the corresponding class definitions.

### Other abstractions

There are many abstractions available in Vulkit. The following is the full list:

- [CommandPool](https://github.com/ismawno/vulkit/blob/main/vulkit/vkit/backend/command_pool.hpp): Makes it a bit easier to allocate and deallocate command buffers, reducing verbosity.
- [SwapChain](https://github.com/ismawno/vulkit/blob/main/vulkit/vkit/backend/swap_chain.hpp): Creating a swap chain can get pretty annoying, specially with the amount of different formats and parameters available. Vulkit uses the same building pattern as the instance and device handles. It also includes the option to create additional, usually handy resources that are needed to be recreated along with the swap chain when the window is resized.
- [Buffers](https://github.com/ismawno/vulkit/tree/main/vulkit/vkit/buffer): Low and high level abstractions for different buffer usages (device local and host visible), as well as quick functions to create vertex, index, uniform and storage buffers
- [Descriptors](https://github.com/ismawno/vulkit/tree/main/vulkit/vkit/descriptors): Three different handles to handle decriptor pools, descriptor set layouts and descriptor writes.
- [Pipelines and Shaders](https://github.com/ismawno/vulkit/tree/main/vulkit/vkit/pipeline): Two different abstractions for graphics and compute pipelines, in addition to a Shader class capable of compiling glsl shaders into the spir-v representation by invoking the glslc compiler.

## Dependencies and Third-Party Libraries

Vulkit relies on some dependencies:

- [toolkit](https://github.com/ismawno/toolkit): A utilities library I have developed.
- [vma](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator): Vulkan Memory Allocator.
- [vulkan](https://github.com/KhronosGroup/Vulkan-Loader): Vulkan API.

## Building

The building process is straightforward. Create a `build` folder, `cd` into it, and run `cmake ..`. All available Vulkit options will be displayed.

Then compile the project with your editor/IDE of choice, and run the tests to make sure everything works as expected. If that is the case, you are done!

## Additional notes

Tests are pretty mediocre, there are almost none.