# Vulkan cheatsheet

## Initialization
## Memory management
Heap - RAM physical stick\
`VkMemoryHeap` is an object that describes one Heap that `VkDevice` can talk to\
`VkMemoryType` describes how to allocate memory\
`VkDevice` has the size of memory in bytes and `VkMemoryHeapFlagBits` memory flags:
* non-local (host memory) - RAM shared between CPU and GPU.
* local (device memory) - VRAM inside GPU

Non-local has 2 memory types, both of them has `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` flag, it allows mapping and allows to choose whether to make cached CPU access with `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` or `VK_MEMORY_PROPERTY_HOST_CACHED_BIT` flags.

Memory heap properties: [https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkMemoryPropertyFlagBits.html](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkMemoryPropertyFlagBits.html)


type - set of memory properties of heap
device memory is organised into heaps
heaps have diifferent types of memory `VkGetPhysicalDeviceMemoryProperties`

local memory - render target / textures


image layout - affects how pixels are organised, different layout are optimal for different tasks:
* `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`: Optimal for presentation
* `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`: Optimal as attachment for writing colors from the fragment shader
* `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`: Optimal as source in a transfer operation, like vkCmdCopyImageToBuffer
* `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`: Optimal as destination in a transfer operation, like vkCmdCopyBufferToImage
* `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`: Optimal for sampling from a shader

### Memory pools
Efficient memory allocation\
Avoids fragmentation\
Single threaded

## Render pipeline
* Shader stages
* Vertex input
* Input assembly
* Viewport state (viewport + scissor)
* Rasterization state
* Color blend state
* Multisampling
* Dynamic state
* Pipeline layout (descriptor layout)

___

1. Create vulkan instance, 
    Select API extensions
    Select hardware (physical device)
2. Create logical device (???)
3. Create window
    Window surface
    Swap chain (double buffering)
4. Receive image from swap chain
    Wrap to image view (use specific part of an image)
    Wrap to frame buffer (used for color, depth and stencil targets)
5. Render passes
    What type of images to use during rendering
6. Graphics pipeline
    Describe configurable state of graphics card
    To change certex layout or shaders need new pipeline
7. Command pools and command buffers
8. Main loop
    Acquire image from swap chain
    Select command buffer 
    Execute command buffer
    Return image to swap chain for presentation


* Create a VkInstance
* Select a supported graphics card (VkPhysicalDevice)
* Create a VkDevice and VkQueue for drawing and presentation
* Create a window, window surface and swap chain
* Wrap the swap chain images into VkImageView
* Create a render pass that specifies the render targets and usage
* Create framebuffers for the render pass
* Set up the graphics pipeline
* Allocate and record a command buffer with the draw commands for every possible swap chain image
* Draw frames by acquiring images, submitting the right draw command buffer and returning the  images back to the swap chain


Check out
RAII, initializer lists

C:/"Program Files (x86)"/"Microsoft SDKs"/Windows/v10.0A/bin/"NETFX 4.7.2 Tools"/ResGen.exe
http://forums.codeblocks.org/index.php?topic=11128.0



shader compile auto
for /r %%i in (*.frag, *.vert) do %VULKAN_SDK%/Bin/glslangValidator.exe -V %%i

`VkInstance` - connection between program and Vulkan library



`vkInstanceCreateInfo` - create application
    * vkApplicationInfo
    * glfwGetRequiredInstanceExtensions
    * validation layers
`vkPhysicalDevice` - select physical device
    * find suitable device
    * find queue families
`vkDevice` - create logical device

  R
G   Y
  B


## General
Vulkan tries to remove guessing for driver.

## Structures
command buffer - buffer where you record commands (`VkCommandBuffer`)
command pool - allocates command buffers, lightweight synchronization

render pass - provide information upfornt
descriptor layout - tell how to use resources, describes what shader can access

queue family - list of queues with the same functionality
queue - abstract mechanism to submit commands to GPU

swapchain - list of image buffers
imageView - wrapper around image, adds extra resources to image that describes how to use it

render pass - A render pass describes the scope of a rendering operation by specifying the collection of attachments, subpasses, and dependencies used during the rendering operation. A render pass consists of at least one subpass. 
render pass is the set of attachments, the way they are used, and the rendering work that is performed using them

renderpass attachment - An attachment corresponds to a single Vulkan VkImageView. A description of the attachment is provided to the render pass creation, which allows the render pass to be configured appropriately; the actual images to be used are provided when the render pass is used, via the VkFrameBuffer. It is possible to associate multiple attachments with a render pass;
More commonly, a color framebuffer and a depth buffer are separate attachments in Vulkan. Therefore the pAttachments member of VkRenderPassCreateInfo points to an array of attachmentCount elements.
A render pass represents a collection of attachments, subpasses, and dependencies between the subpasses, and describes how the attachments are used over the course of the subpasses. The use of a render pass in a command buffer is a render pass instance.

!!!! render pass defines what types of attachments framebuffer will use, and how each subpass will use
renderpass attachments are bound wy wrapping them into `frameBuffer`. buffer references all `imageViews`

### Depth buffer
Create the depth buffer image object\
Allocate the depth buffer device memory\
Bind the memory to the image object\
Create the depth buffer image view

this | is | table
--- | --- | ---
1 | 2 | 3 
`hola` |

a new line
another one
