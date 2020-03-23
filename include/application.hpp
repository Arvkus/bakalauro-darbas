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

        //for(const VkQueueFamilyProperties &prop : queueFamilies) std::cout<< "Queue families: " << std::bitset<4>(prop.queueFlags) << std::endl;

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

    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorPool descriptorPool;
    
    
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // semaphore for each frame
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight; // only required if MAX_FRAMES_IN_FLIGHT > swapchainImages

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    size_t currentFrame = 0;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    const std::vector<Vertex> vertices = {
        Vertex(glm::vec3(-0.5,-0.5, 0.0), glm::vec3(1,0,0), glm::vec2(1,0)),
        Vertex(glm::vec3( 0.5,-0.5, 0.0), glm::vec3(0,1,0), glm::vec2(0,0)),
        Vertex(glm::vec3( 0.5, 0.5, 0.0), glm::vec3(0,0,1), glm::vec2(0,1)),
        Vertex(glm::vec3(-0.5, 0.5, 0.0), glm::vec3(1,1,0), glm::vec2(1,1)),

        Vertex(glm::vec3(-0.5,-0.5,-0.5), glm::vec3(1,0,0), glm::vec2(1,0)),
        Vertex(glm::vec3( 0.5,-0.5,-0.5), glm::vec3(0,1,0), glm::vec2(0,0)),
        Vertex(glm::vec3( 0.5, 0.5,-0.5), glm::vec3(0,0,1), glm::vec2(0,1)),
        Vertex(glm::vec3(-0.5, 0.5,-0.5), glm::vec3(1,1,0), glm::vec2(1,1)),
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
    };

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

        createDepthResources();

        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();

        createTextureImage();
        createTextureImageView();
        createTextureSampler();

        createFrameBuffers();
        createVertexBuffer();
        createIndexBuffer();
        
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();

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

        createDepthResources();

        createRenderPass();

        createGraphicsPipeline();
        createFrameBuffers();
        createUniformBuffers();

        createDescriptorPool(); 
        createDescriptorSets();

        createCommandBuffers();
    }

    //----------------------------------------------------------

    void mainLoop(){

        uint32_t frameCount = 0;
        double timeBegin = glfwGetTime();

        while(!glfwWindowShouldClose(window)) {

            glfwPollEvents();
            drawFrame();
            
            frameCount++;
            if(glfwGetTime() - timeBegin >= 1.0 ){
                std::string title = "Vulkan window (" + std::to_string(frameCount) + ')'; 
                glfwSetWindowTitle(this->window, title.c_str());
                timeBegin = glfwGetTime();
                frameCount = 0;
            }
            
        }

        // wait to empty queues
        vkDeviceWaitIdle(this->device);
    }

    //----------------------------------------------------------

    void cleanupSwapchain(){

        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);
        
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

        for (size_t i = 0; i < swapchainImages.size(); i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }

    void cleanup(){
        cleanupSwapchain();

        vkDestroySampler(device, textureSampler, nullptr);

        vkDestroyImageView(device, textureImageView, nullptr);
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

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
        appInfo.apiVersion = VK_API_VERSION_1_0;

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

        VkPhysicalDeviceFeatures deviceFeatures = {}; // get physical device features that are supported for logical device
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        printf("%s | %s | ",deviceProperties.deviceName, apiVersionToString(deviceProperties.apiVersion).c_str());

        switch(deviceProperties.deviceType){
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   std::cout<< "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU"   << std::endl; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: std::cout<< "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU" << std::endl; break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            std::cout<< "VK_PHYSICAL_DEVICE_TYPE_CPU"            << std::endl; break;
            default:                                     std::cout<< "VK_PHYSICAL_DEVICE_TYPE_OTHER"          << std::endl; break;
        }
        
        // optional - select memory type?
        VkPhysicalDeviceMemoryProperties memoryProperties = {};
        vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
        printf("Heaps: %i | Types: %i \n", memoryProperties.memoryHeapCount, memoryProperties.memoryTypeCount);

        for(int i = 0; i < memoryProperties.memoryHeapCount; i++){
            const VkMemoryHeap &heap = memoryProperties.memoryHeaps[i];
            std::cout<< "Heap " << i << " size: " << heap.size << " bytes" << std::endl;
        }

        /* VkMemoryPropertyFlagBits:
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 0x00000001,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 0x00000002,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 0x00000004,
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT = 0x00000008,
        */

        // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkMemoryPropertyFlagBits.html
        for(int i = 0; i < memoryProperties.memoryTypeCount; i++){
            const VkMemoryType &type = memoryProperties.memoryTypes[i];
            std::cout<< "Mem: " << std::setw(2) << i << " heap: " << type.heapIndex << " | flags: " << std::bitset<8>(type.propertyFlags) << std::endl;
        }
        //4232126464
        //4294967296

        //heaps = memoryProperties.memoryHeaps;

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

        return this->queues.isComplete() && isExtensionsSupported && swapChainAdequate && swapChainAdequate && deviceFeatures.samplerAnisotropy;
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
        deviceFeatures.samplerAnisotropy = VK_TRUE;

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
            swapchainImageViews[i] = createImageView(this->swapchainImages[i], this->swapChainDetails.chooseSwapSurfaceFormat().format, VK_IMAGE_ASPECT_COLOR_BIT);
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
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

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

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
        renderPassInfo.pAttachments = attachments.data();
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

    void createDescriptorSetLayout(){
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional // relevant for image sampling related descriptors

        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

        VkDescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        createInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
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
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; 

        vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); 

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
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE; // compare depth of new frags with depth buffer if they should be discarded
        depthStencil.depthWriteEnable = VK_TRUE; // if they pass depth test, write to depth buffer
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

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

        // set uniform values (descriptor set layout)
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {}; // Shader Bindings
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &this->descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, &this->pipelineLayout) != VK_SUCCESS) {
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
        pipelineInfo.pDepthStencilState = &depthStencil;
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

            std::array<VkImageView, 2> attachments = { this->swapchainImageViews[i], depthImageView};
            VkFramebufferCreateInfo framebufferInfo = {};

            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = this->renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
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
        /*
            command pool - manage the memory that is used to store the buffers, buffers are allocated to pool
        */

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

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = {0.0, 0.0, 0.0, 1.0};
        clearValues[1].depthStencil = {1.0, 0};

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
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            // VK_SUBPASS_CONTENTS_INLINE - render pass commands will be embedded in the primary command buffer itself, no secondary buffers.
            // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS - The render pass commands will be executed from secondary command buffers.
            // vkCmd - prefix to call commands

            // VK_PIPELINE_BIND_POINT_GRAPHICS - specify if graphics or compute pipeline
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphicsPipeline);

            VkBuffer vertexBuffers[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
            //vkCmdDraw(commandBuffers[i], (uint32_t)vertices.size(), 1, 0, 0);
            vkCmdDrawIndexed(commandBuffers[i], (uint32_t)indices.size(), 1, 0, 0, 0);
            
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

        updateUniformBuffer(imageIndex);

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

    void updateUniformBuffer(uint32_t currentImage){
        UniformBufferObject ubo = {};
        ubo.model = glm::mat4(1.0); //glm::rotate(glm::mat4(1.0f), glfwGetTime() * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::mat4(1.0);
        ubo.proj = glm::perspective(
            glm::radians(45.0f), 
            (float) swapChainDetails.chooseImageExtent().width / (float) swapChainDetails.chooseImageExtent().height,
            0.1f, 100.0f
        );
        ubo.proj[1][1] *= -1;


        float angle = glfwGetTime()*15;
        float distance = 2;
        float x = glm::sin( glm::radians(angle) ) * distance;
        float y = glm::cos( glm::radians(angle) ) * distance;

        ubo.view = glm::lookAt(glm::vec3(x,y,2), glm::vec3(0,0,0), glm::vec3(0,0,1));
        ubo.model = glm::translate(ubo.model, glm::vec3(0,0,.5));

        void* data;
        vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
    }

    void createVertexBuffer(){
        // https://stackoverflow.com/questions/36436493/could-someone-help-me-understand-vkphysicaldevicememoryproperties
        /*
            create vk resource
            allocate memory
            bind memory to resource
        */

       VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
       createBuffer(
           bufferSize,
           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
           this->vertexBuffer,
           this->vertexBufferMemory
       );

        // fill memory
        void* data; // memory location to where to copy
        vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize); // destination, source, size
        vkUnmapMemory(device, vertexBufferMemory);
    }

    void createIndexBuffer(){
       VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
       createBuffer(
           bufferSize,
           VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
           this->indexBuffer,
           this->indexBufferMemory
       );

        // fill memory
        void* data; // memory location to where to copy
        vkMapMemory(device, indexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t) bufferSize); // destination, source, size
        vkUnmapMemory(device, indexBufferMemory);
    }

    void createUniformBuffers(){

        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(swapchainImages.size());
        uniformBuffersMemory.resize(swapchainImages.size());    

         for (size_t i = 0; i < swapchainImages.size(); i++) {
            createBuffer(
                bufferSize, 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                uniformBuffers[i], 
                uniformBuffersMemory[i]
            );
        }

    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory ){

        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.size = size;
        createInfo.usage = usage;

        if(vkCreateBuffer(this->device, &createInfo, nullptr, &buffer) != VK_SUCCESS){
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements = {};
        vkGetBufferMemoryRequirements(this->device, buffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if(vkAllocateMemory(this->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS){
            throw std::runtime_error("failed to allocate memory!");
        };

        vkBindBufferMemory(this->device, buffer, bufferMemory, 0);
    }

    uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags requiredProperties) {

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            const VkMemoryPropertyFlags& supportedProperties = memProperties.memoryTypes[i].propertyFlags;
            //std::cout<< "memory search: " << std::bitset<16>(typeBits) << " (" << typeBits << ") & " << std::bitset<16>(1 << i) << 
            //" && " << std::bitset<16>(properties) << " & " << std::bitset<16>(supportedProperties) <<std::endl;
            if (typeBits & (1 << i) && (supportedProperties & requiredProperties) == requiredProperties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createDescriptorPool(){
        std::array<VkDescriptorPoolSize, 2> poolSizes = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchainImages.size());
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchainImages.size());

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(swapchainImages.size());

        if (vkCreateDescriptorPool(this->device, &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets(){
        std::vector<VkDescriptorSetLayout> layouts(swapchainImages.size(), this->descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = this->descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchainImages.size());
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(swapchainImages.size());
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        printf("allocated descriptor sets \n");

        for (size_t i = 0; i < swapchainImages.size(); i++) {

            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;


            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        }
    }

    void createTextureImage(){

        // read image
        int width, height, channel;
        stbi_uc* pixels = stbi_load("textures/image.jpg", &width, &height, &channel, STBI_rgb_alpha);
        if(!pixels) throw std::runtime_error("failed to load texture image!");
        VkDeviceSize memorySize = width * height *4;

        // create buffer on RAM
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(
            memorySize, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            stagingBuffer, 
            stagingBufferMemory
        );

        // move pixels to buffer memory (RAM);
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, memorySize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(memorySize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);

        // create image memory on VRAM
        createImage(
            width, height, 
            VK_FORMAT_R8G8B8A8_SRGB, 
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            this->textureImage,
            this->textureImageMemory
        );

        // move pixels from buffer to image memory
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, this->textureImage, width, height);
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(this->device, stagingBuffer, nullptr);
        vkFreeMemory(this->device, stagingBufferMemory, nullptr);
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImage& image, VkDeviceMemory& imageMemory){
                
        VkImageCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.extent.width = width;
        createInfo.extent.height = height;
        createInfo.extent.depth = 1;
        createInfo.mipLevels = 1;
        createInfo.arrayLayers = 1;
        createInfo.format = format; // VK_FORMAT_R8G8B8A8_SRGB
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // (or VK_IMAGE_TILING_LINEAR) efficient texel tiling
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // VK_IMAGE_LAYOUT_PREINITIALIZED - Not usable by the GPU and the very first transition will discard the texels.
        // VK_IMAGE_LAYOUT_PREINITIALIZED - Not usable by the GPU, but the first transition will preserve the texels.
        createInfo.usage = usage; // VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        // VK_IMAGE_USAGE_SAMPLED_BIT - access usage for shader
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.flags = 0; // Optional

        if (vkCreateImage(device, &createInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    void createTextureImageView(){
        this->textureImageView = createImageView(this->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags){

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format; // VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = aspectFlags; // VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;

    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(this->queues.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(this->queues.graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        barrier.srcAccessMask = 0; // TODO
        barrier.dstAccessMask = 0; // TODO

        //-------------------------------------------------------------

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        //-------------------------------------------------------------

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        endSingleTimeCommands(commandBuffer);
    }

    void createTextureSampler(){
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR; // or VK_FILTER_NEAREST
        samplerInfo.minFilter = VK_FILTER_LINEAR; // or VK_FILTER_NEAREST

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16;

        samplerInfo.unnormalizedCoordinates = VK_FALSE; // use normalized [0, 1]

        // https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
        samplerInfo.compareEnable = VK_FALSE; // for shadow mapping
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void createDepthResources(){
        VkFormat depthFormat = findDepthFormat();

        printf("creating depth \n");
        createImage(
            swapChainDetails.chooseImageExtent().width, 
            swapChainDetails.chooseImageExtent().height, 
            depthFormat, 
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
            depthImage, 
            depthImageMemory
        );
            
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {

        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

         throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

};


// render pipeline, render pass, subpass, framebuffer - realationship
// semaphores, drawing


