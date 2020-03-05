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


