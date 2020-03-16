#include "common.hpp"

// https://vulkan.lunarg.com/doc/view/1.0.57.0/windows/validation_layers.html
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

// device extensions and instance extensions
const std::vector<const char*> deviceExtensions = {
    "VK_KHR_swapchain", // VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct SwapChainSupportDetails {


    VkSurfaceCapabilitiesKHR capabilities; // min/max number of images in chain + image width/height
    std::vector<VkSurfaceFormatKHR> formats; // pixel formats, color space
    std::vector<VkPresentModeKHR> presentModes; // available present modes

    //VkSurfaceFormatKHR surfaceFormat;
    //VkPresentModeKHR presentMode;
    //VkExtent2D extent;

    //----------------------------------

    bool querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &this->capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            this->formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, this->formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            this->presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, this->presentModes.data());
        }

        //-------------------------------------------------------------------
        // formats: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkFormat.html
        // colors:  https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkColorSpaceKHR.html
        for(VkSurfaceFormatKHR format : this->formats){
            //printf("format: %i | colors: %i \n", format.format, format.colorSpace);
        }

        // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPresentModeKHR.html
        for(VkPresentModeKHR mode : this->presentModes){
            //printf("mode: %i \n", mode);
        }

        return !this->formats.empty() && !this->presentModes.empty();
    }

    //----------------------------------
    // pick format and color space

    VkSurfaceFormatKHR chooseSwapSurfaceFormat() {
        for (const auto& availableFormat : this->formats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return formats[0];
    } 

    //----------------------------------
    // present mode

    VkPresentModeKHR chooseSwapPresentMode() {
        for (const auto& availablePresentMode : this->presentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                printf("present mode: VK_PRESENT_MODE_MAILBOX_KHR \n");
                /*
                When a new image is presented, it is marked as the pending image, 
                and at the next opportunity (probably after the next vertical refresh), the system will display it to the user. 
                If a new image is presented before this happens, that image will be shown, and the previously presented image will be discarded.
                Maximum latency
                */
                return availablePresentMode;
            }
        }

        printf("present mode: VK_PRESENT_MODE_FIFO_KHR \n");
        return VK_PRESENT_MODE_FIFO_KHR; // V-sync
    }

    //---------------------------------- 
    // dimensions of an image

    VkExtent2D chooseImageExtent() {
        //printf("capabilities: %i %i \n", this->capabilities.currentExtent.width, this->capabilities.currentExtent.height);
       
        if (this->capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            // glfwGetWindowSize(window, &width, &height);
            VkExtent2D actualExtent = {800, 600};

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    //---------------------------------- 

    uint32_t chooseImageCount(){
        uint32_t imageCount = 3;

        // 0 - means there's no maximum
        if(this->capabilities.maxImageCount == 0) return imageCount;
        if(this->capabilities.maxImageCount < imageCount){
            printf("Device only supports only %i images in swapchain", imageCount);
            return this->capabilities.maxImageCount;
        }

        return imageCount;
    }

    //---------------------------------- 

};

struct Queues {
public:
    std::optional<uint32_t> graphicsFamilyIndex; // draw calls family
    std::optional<uint32_t> presentFamilyIndex;  // presentation family (display imgs on surface)

    VkQueue presentQueue;
    VkQueue graphicsQueue;

    bool isComplete() { 
        return graphicsFamilyIndex.has_value() && presentFamilyIndex.has_value(); 
    }

    void findFamilies(VkPhysicalDevice device, VkSurfaceKHR surface){

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        /*
            0001 // VK_QUEUE_GRAPHICS_BIT       // drawing points, lines, and triangles
            0010 // VK_QUEUE_COMPUTE_BIT        // dispatching compute shaders
            0100 // VK_QUEUE_TRANSFER_BIT       // copying buffer and image contents
            1000 // VK_QUEUE_SPARSE_BINDING_BIT // update sparse resources
        */

        // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkQueueFlagBits.html
        int index = 0;
        for (const auto& queueFamily : queueFamilies) {

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupport);
            if(presentSupport) presentFamilyIndex = index;

            if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT){
                graphicsFamilyIndex = index;
            }

            if(this->isComplete()) break;

            // std::cout<< index << " " << std::bitset<8>(queueFamily.queueFlags)<< " " << queueFamily.queueCount <<std::endl;
            index++;
        }
    }

};

//---------------------------------------------------------



class Application {
public:
    
    void run() {
        initWindow();

        uint64_t start = timeSinceEpochMicro();
        initVulkan();
        uint64_t finish = timeSinceEpochMicro();
        std::cout<< "Vulkan load time: " << (float)(finish - start) / 1000000 << std::endl;

        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance; 
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    Queues queues;

    SwapChainSupportDetails swapChainDetails;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkRenderPass renderPass;

    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // semaphore for each frame
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight; // only required if MAX_FRAMES_IN_FLIGHT > swapchainImages

    size_t currentFrame = 0;

    void initWindow(){
        glfwInit();
    
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // disable OpenGL
        // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // disable resize 
        this->window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

        // set pos to middle
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowPos(window, (mode->width - 800) / 2, (mode->height - 600) / 2 );

        //glfwSetWindowSizeCallback(window, size_callback);
    }

    void initVulkan(){
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFrameBuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void recreate_swapchain(){

        int width = 0, height = 0; // window minimize case
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        printf("\n--- swapchain recreation ---\n");
        vkDeviceWaitIdle(this->device);

        cleanupSwapchain();

        createSwapchain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFrameBuffers();
        createCommandBuffers();
    }

    void mainLoop(){
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        // wait to empty queues
        vkDeviceWaitIdle(this->device);
    }

    void cleanupSwapchain(){
        
        // free command buffers because they relate to framebuffers
        vkFreeCommandBuffers(this->device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

        for (const auto &framebuffer : swapchainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (const auto &imageView : swapchainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(this->device, this->swapchain, nullptr);
    }

    void cleanup(){
        cleanupSwapchain();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(this->device, this->commandPool, nullptr);

        vkDestroyDevice(this->device, nullptr);
        vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
        vkDestroyInstance(this->instance, nullptr);
        glfwDestroyWindow(this->window);
        glfwTerminate();
    }

    //------------------------------------------------------------------------------------

    void createInstance(){  

        // Application metadata
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan visualizer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;

        // get vulkan extensions for instance (glfw built-in function)
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        
        // create info of instance
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        // enable validation layers if debug mode
        if(isDebug){
            // check if validation layers are supported
            if(!isValidationLayersSupported()) throw std::runtime_error("validation layers are not supported");

            createInfo.enabledLayerCount = static_cast<int32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }else{
            createInfo.enabledLayerCount = 0;
        }

        if(vkCreateInstance(&createInfo, nullptr, &this->instance) == VK_SUCCESS){
            printf("Created Vulkan instance \n");
        }else{
            throw std::runtime_error("failed to create instance");
        }
         
    }

    bool isValidationLayersSupported(){
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        //for(VkLayerProperties &layer : availableLayers)printf("%s | %s \n", layer.layerName, layer.description);

        for(const char* requiredLayer : validationLayers){
            bool layerFound = false;

            for(const VkLayerProperties& layerInfo : availableLayers){
                if(std::strcmp(layerInfo.layerName, requiredLayer) == 0 ){
                    layerFound = true;
                }
            }

            if(!layerFound){
                printf("%s - validation layer is not supported \n", requiredLayer );
                return false;
            };
        }

        return true;
    }

    void createSurface(){
        if (glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface) == VK_SUCCESS) {
            printf("Created Vulkan surface \n");
        }else{
            throw std::runtime_error("failed to create window surface!");
        }
    }

    //-----------------------------------------------------------------------------
    // Physical device selection

    void pickPhysicalDevice(){

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());

        printf("\nFound physical (%i) devices: \n", deviceCount);
        for (const VkPhysicalDevice& device : devices) {
            
            if (isDeviceSuitable(device)) {
                this->physicalDevice = device;
                printf("Device is suitable \n");
                break; // pick first available
            }
        }

    }

    bool isDeviceSuitable(VkPhysicalDevice device) {

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        //VkPhysicalDeviceFeatures deviceFeatures; // get physical device features that are supported for logical device
        //vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        printf("%s | %s | ",deviceProperties.deviceName, apiVersionToString(deviceProperties.apiVersion).c_str());

        switch(deviceProperties.deviceType){
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   std::cout<< "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU"   << std::endl; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: std::cout<< "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU" << std::endl; break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            std::cout<< "VK_PHYSICAL_DEVICE_TYPE_CPU"            << std::endl; break;
            default:                                     std::cout<< "VK_PHYSICAL_DEVICE_TYPE_OTHER"          << std::endl; break;
        }
        
        // optional - select memory type?
        // VkPhysicalDeviceMemoryProperties memoryProperties = {};
        // vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
        // printf("Heaps: %i | Types: %i", memoryProperties.memoryHeapCount, memoryProperties.memoryTypeCount);

        // Vulkan devices execute work that is submitted to queues.
        // Queue belongs to QueueFamily
        // QueueFamily is a group of queues that have identical capabilities but are able to run in parallel
        // The number of families, family capabilities, number of queues for each family are properties of the PhysicalDevice

        // find families that can do draw commands and present image
        this->queues.findFamilies(device, this->surface); // get queue family index for logical device
        bool isExtensionsSupported = isDeviceExtensionsSupported(device); 
        

        // check if physical device and surface supports required swapchain features
        bool swapChainAdequate = false;

        this->swapChainDetails = SwapChainSupportDetails();
        if (isExtensionsSupported ) {
            swapChainAdequate = this->swapChainDetails.querySwapchainSupport(device, this->surface);
        }

        return this->queues.isComplete() && isExtensionsSupported && swapChainAdequate && swapChainAdequate;
    }

    bool isDeviceExtensionsSupported(const VkPhysicalDevice &device){
        // need swapchain extension
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // what extensions device supports
        // for(VkExtensionProperties &properties : availableExtensions) printf("%s\n", properties.extensionName);

        bool isSupported = true;
        for(const char* requiredExtensionName : deviceExtensions){
            bool extensionFound = false;

            for (const VkExtensionProperties &availableExtension : availableExtensions) {
                if( std::strcmp(requiredExtensionName, availableExtension.extensionName) == 0 ) extensionFound = true;
            }

            if(extensionFound == false){
                printf("Extension is not supported: %s\n", requiredExtensionName);
                isSupported = false;
            }
        }
        
        return isSupported;
    }

    //-----------------------------------------------------------------------------

    void createLogicalDevice(){

        // prepeare queue create info for logical device
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            this->queues.graphicsFamilyIndex.value(), 
            this->queues.presentFamilyIndex.value(),
        };

        float priority = 1.0;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &priority; // [0.0; 1.0]
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // select device features from physical device
        VkPhysicalDeviceFeatures deviceFeatures = {}; // contains optional shaders...

        // create logical device with queues
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if(vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device) == VK_SUCCESS){
            std::cout << "Successfully created logical device!" << std::endl;
        }else{
            throw std::runtime_error("failed to create logical device!");
        };

        // get queues from logical device
        vkGetDeviceQueue(device, this->queues.graphicsFamilyIndex.value(), 0, &this->queues.graphicsQueue);
        vkGetDeviceQueue(device, this->queues.presentFamilyIndex.value(), 0, &this->queues.presentQueue);
    }

    //-----------------------------------------------------------------------------
    /* 
    memory:
        host memory - objects created by api
        device memory - resource objects (keep allocation objects to a minimum)
    */

    void createSwapchain(){
        this->swapChainDetails.querySwapchainSupport(this->physicalDevice, this->surface); // get new swapchain details
        
        //  this could be: check for support, fill create info with hard-coded values
        VkSurfaceFormatKHR surfaceFormat = this->swapChainDetails.chooseSwapSurfaceFormat();
        uint32_t imageCount = this->swapChainDetails.chooseImageCount();

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = this->surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = this->swapChainDetails.chooseImageExtent();
        printf("Extent size: %i %i \n", createInfo.imageExtent.width, createInfo.imageExtent.height);

        createInfo.imageArrayLayers = 1; // always 1 layer, 2 layers are for stereoscopic 3D application
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // specifies that the image can be used to create a VkImageView, use as a color

        // specify if transform should be applied, currentTransfrom = no transform
        // VkSurfaceTransformFlagBitsKHR enumeration
        createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; //this->swapChainDetails.capabilities.currentTransform; 

        // alpha channel use with other windows, ignore it
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; 

        createInfo.presentMode = this->swapChainDetails.chooseSwapPresentMode();
        createInfo.clipped = VK_TRUE; // if pixels are obscured, clip them
        createInfo.oldSwapchain = VK_NULL_HANDLE; // VK_NULL_HANDLE or swapchain; // if window resizes, need to recreate swapchain

        uint32_t queueFamilyIndices[] = {
            this->queues.graphicsFamilyIndex.value(),
            this->queues.presentFamilyIndex.value(),
        };

        // printf("%i %i \n", this->queues.presentFamilyIndex.value(), this->queues.graphicsFamilyIndex.value());

        if (this->queues.graphicsFamilyIndex != this->queues.presentFamilyIndex) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            // image going to be used by 1 queue
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }
        
        // swap chain also auto-creates images
        VkResult result = vkCreateSwapchainKHR(this->device, &createInfo, nullptr, &this->swapchain);
        if (result == VK_SUCCESS) {
            printf("Created swapchain \n");
        }else{
            std::cout<< "swapchain error code: " << result << std::endl; // VK_ERROR_VALIDATION_FAILED_EXT
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(this->device, this->swapchain, &imageCount, nullptr); // image count can be different ? TODO: 0?
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(this->device, this->swapchain, &imageCount, swapchainImages.data());        
        
        // TODO: make these global variables
        //VkFormat swapChainImageFormat = surfaceFormat.format;
        //VkExtent2D swapChainExtent = this->swapChainDetails.chooseImageExtent();
    }

    void createImageViews(){
        swapchainImageViews.resize(this->swapchainImages.size());

        for (size_t i = 0; i < swapchainImages.size(); i++) {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = this->swapChainDetails.chooseSwapSurfaceFormat().format;

            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }

        printf("Created image views \n");
    }

    void createRenderPass(){
        /*
            Render pass is set of attachments, dependencies, subpasses
            Attachment - describe how content is treated at the start/end of each render pass instance, including format, sample count
            Subpass - render commands are recorded into supbass
            Dependency - describe execution and memory dependencies between subpasses.

            Attacjment types: output (color, depth/stencil), input, resolve (no read/write, preserve content)

            The actual images to be used are provided when the render pass is used, via the VkFrameBuffer
            Render pass is wrapped into framebuffer
        */

        VkAttachmentDescription colorAttachment = {}; 
        colorAttachment.format = this->swapChainDetails.chooseSwapSurfaceFormat().format; // VK_FORMAT_B8G8R8A8_SRGB
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // number of samples

        // what to do with data before/after rendering
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Rendered contents will be stored in memory and can be read later

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        //----------------------

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        //----------------------
        // specify memory and execution dependencies between subpasses
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;

        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        //----------------------

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment; 
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }else{
            std::cout<< "Render pass created successfully" << std::endl;
        }

    }

    void createGraphicsPipeline(){
        // dynamic things in pipeline: size of the viewport, line width and blend constants
        // stages - https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPipelineStageFlagBits.html

        std::vector<char> vertShaderCode = readFile("shaders/vert.spv");
        std::vector<char> fragShaderCode = readFile("shaders/frag.spv");

        // wrap code in shader module to pass it into pipeline
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main"; // standart entry point, possible to combine multiple shaders with different points

        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};


        // define if data is per-vertex or per-instance, what data to load
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional , details for loading verticex
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional , details for loading verticex

        // what geometry to draw, and if primitive restart should be enabled
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // triangle from every 3 vertices without reuse
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        /*
            viewport - shrink image
            scissor - crop image
        */
        // (0, 0) to (width, height)
        VkViewport viewport = {}; 
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) this->swapChainDetails.chooseImageExtent().width;
        viewport.height = (float) this->swapChainDetails.chooseImageExtent().height;
        viewport.minDepth = 0.0f; // depth values for framebuffer
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {}; 
        scissor.offset = {0, 0};
        scissor.extent = this->swapChainDetails.chooseImageExtent();

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // also performs depth testing and face culling
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; // if fragments are beyond far/near planes, they are clamped
        rasterizer.rasterizerDiscardEnable = VK_FALSE; // if true, geomentry never passes through rasterizer
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // fill triangles
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE; // for shadow mapping

        // anti aliasing, requeres to enable GPU feature
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; // state per attatched framebuffer
        // what channels to pass
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE; // new color from the fragment shader is passed through unmodified

        // set blend constants that you can use as blend factors in the aforementioned calculations.
        VkPipelineColorBlendStateCreateInfo colorBlending = {}; // global color blending settings
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;// Optional
        colorBlending.blendConstants[1] = 0.0f;// Optional
        colorBlending.blendConstants[2] = 0.0f;// Optional
        colorBlending.blendConstants[3] = 0.0f;// Optional

        // set uniform values
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }else{
            std::cout<< "Successfully created pipeline layout" << std::endl;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        // required for switching between multiple pipelines
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }else{
            std::cout<< "Successfully created graphics pipelines" << std::endl;
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);

    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    void createFrameBuffers(){
        // printf("imageviews: %i \n", (int)swapchainImageViews.size());

        // Renderpass use framebuffers

        this->swapchainFramebuffers.resize(this->swapchainImageViews.size());

        for (size_t i = 0; i < this->swapchainImageViews.size(); i++) {

            VkImageView attachments[] = { this->swapchainImageViews[i] /* framebuffer view here? */ };
            VkFramebufferCreateInfo framebufferInfo = {};

            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = this->renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = this->swapChainDetails.chooseImageExtent().width;
            framebufferInfo.height = this->swapChainDetails.chooseImageExtent().height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }

        printf("Created framebuffers \n");
    }

    void createCommandPool(){
        // operations and memory transfers

        // cant use commands directly
        // record all operations i want to use
        // setting up commands can be done in advance (hard work)

        // command pool - manage the memory that is used to store the buffers, buffers are allocated to pool

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queues.graphicsFamilyIndex.value();
        poolInfo.flags = 0; // Optional
        
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &this->commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    };

    void createCommandBuffers(){
        /*
            device -> 
            surface capabilities image count -> 
            swapchain Images -> 
            swapchain imageViews -> 
            swapchain frameBuffers -> 
            swapchain commandBuffers
        */
        this->commandBuffers.resize(swapchainFramebuffers.size());
        VkCommandBufferAllocateInfo allocInfo = {};

        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // primary or secondary
        // primary - can be submitted to queue, but can't be called from other command buffers
        // secondary - cannot be submitted directly, but can be called from primary buffer
        allocInfo.commandBufferCount = (uint32_t) this->commandBuffers.size();

        if (vkAllocateCommandBuffers(this->device, &allocInfo, this->commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        printf("Allocated command buffers \n");

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f}; // clear color

        // start to record commands
        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional // for secondary command buffers

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            // RECORD START

            // recording starts with render pass, create info
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = this->renderPass;
            renderPassInfo.framebuffer = this->swapchainFramebuffers[i]; // framebuffer for each swap chain image that specifies it as color attachment.
            renderPassInfo.renderArea.offset = {0, 0}; // render area
            renderPassInfo.renderArea.extent = this->swapChainDetails.chooseImageExtent();
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            // VK_SUBPASS_CONTENTS_INLINE - render pass commands will be embedded in the primary command buffer itself, no secondary buffers.
            // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS - The render pass commands will be executed from secondary command buffers.
            // vkCmd - prefix to call commands

            // VK_PIPELINE_BIND_POINT_GRAPHICS - specify if graphics or compute pipeline
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphicsPipeline);
            vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
            
            vkCmdEndRenderPass(commandBuffers[i]);

             // RECORD END

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        printf("Recorded render commands \n");
    };

    void createSyncObjects(){
        // semaphores are designed to be used to sync queues, GPU-GPU
        // fences are designed to sync vulkan, GPU-CPU
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

       
            if (vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(this->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores!");
            }
        }

        printf("Created sync objects \n");
    }


    void drawFrame(){
        // works like a debounce
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        // take image from the swapchain
        uint32_t imageIndex;
        VkResult result;
        
        result = vkAcquireNextImageKHR(this->device, this->swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if(result == VK_ERROR_OUT_OF_DATE_KHR){
            printf("swapchain out of date \n");
            this->recreate_swapchain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // submit command buffer to queue
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(this->queues.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) { // submit command buffer to queue
            throw std::runtime_error("failed to submit draw command buffer!");
        }


        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {this->swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        presentInfo.pResults = nullptr; // Optional

        result = vkQueuePresentKHR(this->queues.presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            this->recreate_swapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    };

};


// render pipeline, render pass, subpass, framebuffer - realationship
// semaphores, drawing


