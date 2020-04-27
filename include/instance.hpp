#pragma once
#include "common.hpp"

namespace{
    struct Queues{
        VkQueue present_queue;
        VkQueue graphics_queue;
        VkQueue compute_queue;
        VkQueue transfer_queue;

        uint32_t present_family_index; // Surface imgs on surface
        uint32_t graphics_family_index;
        uint32_t compute_family_index;
        uint32_t transfer_family_index;
    };

    struct Surface{ // supported surface info.
        VkSurfaceKHR vulcan_surface;
        VkSurfaceCapabilitiesKHR capabilities;
        VkPresentModeKHR present_mode;
        VkSurfaceFormatKHR surface_format;
        uint32_t image_count;
        VkFormat depth_format;
    };
}

class Instance{
public:

    GLFWwindow* window;

    VkInstance vulkan_instance;
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkCommandPool transfer_command_pool;
    
    Queues queues = {};
    Surface surface = {};

    void init(GLFWwindow* window)
    {
        this->window = window;
        create_instance();
        create_surface();
        pick_physical_device();
        pick_device_queues();
        create_device();
        create_transfer_command_pool();
    }

    void destroy()
    {
        vkDestroyCommandPool(this->device, transfer_command_pool, nullptr);
        vkDestroySurfaceKHR(this->vulkan_instance, this->surface.vulcan_surface, nullptr);
        vkDestroyDevice(this->device, nullptr);
        vkDestroyInstance(this->vulkan_instance, nullptr);
    }

    void update_surface_capabilities() // if surface changes, update required
    {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->physical_device, this->surface.vulcan_surface, &this->surface.capabilities);
    }

    VkCommandBuffer begin_single_use_command() {
        VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; 
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = transfer_command_pool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void end_single_use_command(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queues.transfer_queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queues.transfer_queue);

        vkFreeCommandBuffers(device, transfer_command_pool, 1, &commandBuffer);
    }

private:

    //------------------------------------------------------------------------------------------------------------------------------

    void create_instance()
    {

        // debug mode enabled, but doesn't support validation layers
        if(APP_DEBUG && !is_validation_layers_supported()){
            throw std::runtime_error("validation layers are not supported");
        } 

        uint32_t glfw_extension_count = 0;
        const char** glfw_extension_names;
        glfw_extension_names = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        VkApplicationInfo ai = {};
        ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        ai.pApplicationName = TITLE;
        ai.apiVersion = VK_API_VERSION_1_1;

        VkValidationFeatureEnableEXT enables[] = {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT};
        VkValidationFeaturesEXT features = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
        features.enabledValidationFeatureCount = 2;
        features.pEnabledValidationFeatures = enables;

        VkInstanceCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.pApplicationInfo = &ai;
        ci.enabledExtensionCount = glfw_extension_count;
        ci.ppEnabledExtensionNames = glfw_extension_names;
        ci.enabledLayerCount = APP_DEBUG? (uint32_t)VALIDATION_LAYERS.size() : 0;
        ci.ppEnabledLayerNames = APP_DEBUG? VALIDATION_LAYERS.data(): nullptr;
        ci.pNext = &features;

        if(vkCreateInstance(&ci, nullptr, &this->vulkan_instance) == VK_SUCCESS){
            printf("Created Vulkan instance \n");
        }else{
            throw std::runtime_error("failed to create instance");
        }
    }

    void create_surface(){
        if (glfwCreateWindowSurface(this->vulkan_instance, this->window, nullptr, &this->surface.vulcan_surface) == VK_SUCCESS) {
            printf("Created Vulkan surface \n");
        }else{
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void pick_physical_device()
    {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(vulkan_instance, &device_count, nullptr);
        if(device_count == 0) throw std::runtime_error("could not find physical device");
        std::vector<VkPhysicalDevice> physical_devices(device_count);
        vkEnumeratePhysicalDevices(vulkan_instance, &device_count, physical_devices.data());

        // pick first device
        if(!is_device_compatible(physical_devices[0])) throw std::runtime_error("selected device is not compatible");
        if(!is_device_surface_capable(physical_devices[0])) throw std::runtime_error("surface is not capable for rendering");
        this->physical_device = physical_devices[0]; 
    }

    void pick_device_queues()
    {   
        // Vulkan devices execute work that is submitted to queues.
        // Queue belongs to QueueFamily
        // QueueFamily is a group of queues that have identical capabilities but are able to run in parallel
        // The number of families, family capabilities, number of queues for each family are properties of the PhysicalDevice

        /*
            0001 // VK_QUEUE_GRAPHICS_BIT       // drawing points, lines, and triangles
            0010 // VK_QUEUE_COMPUTE_BIT        // dispatching compute shaders
            0100 // VK_QUEUE_TRANSFER_BIT       // copying buffer and image contents
            1000 // VK_QUEUE_SPARSE_BINDING_BIT // update sparse resources
        */

        uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(this->physical_device, &family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(this->physical_device, &family_count, queue_families.data());

        // for(const VkQueueFamilyProperties &prop : queue_families) std::cout<< "Queue families: " << std::bitset<4>(prop.queueFlags) << std::endl;

        uint32_t index = 0;
        VkBool32 is_complete = false;

        for(const VkQueueFamilyProperties& queue_family : queue_families){
            VkBool32 is_surface_supported = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(this->physical_device, index, this->surface.vulcan_surface, &is_surface_supported);

            // 1 family queue must support everything
            if(is_surface_supported 
            && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT 
            && queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT
            && queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT){
                this->queues.present_family_index = index;
                this->queues.graphics_family_index = index;
                this->queues.compute_family_index = index;
                this->queues.transfer_family_index = index;
                is_complete = true;
            } 
            index++;
        }

        if(is_complete){
            printf("Queue families are supported \n");
        }else{
            throw std::runtime_error("required queue family is not supported");
        }
    }

    void create_device()
    {
        // queue create infos for logical device
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> queue_families = { // set doesn't allow duplicates
            this->queues.present_family_index,
            this->queues.graphics_family_index,
            this->queues.compute_family_index,
            this->queues.transfer_family_index,
        };

        float priority = 1.0;
        for(uint32_t index : queue_families){
            VkDeviceQueueCreateInfo ci = {};
            ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            ci.queueFamilyIndex = index;
            ci.queueCount = 1;
            ci.pQueuePriorities = &priority; // [0.0 ; 1.0]
            queue_create_infos.push_back(ci);
        }

        // select device features from physical device
        VkPhysicalDeviceFeatures device_features = {};
        device_features.samplerAnisotropy = VK_TRUE;

        // create logical device with queues
        VkDeviceCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        ci.pEnabledFeatures = &device_features;
        ci.queueCreateInfoCount = (uint32_t)queue_create_infos.size();
        ci.pQueueCreateInfos = queue_create_infos.data();
        ci.enabledExtensionCount = (uint32_t)DEVICE_EXTENSIONS.size();
        ci.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

        if(vkCreateDevice(this->physical_device, &ci, nullptr, &this->device) == VK_SUCCESS){
            printf("Created logical device \n");
        }else{
            throw std::runtime_error("failed to create logical device!");
        }

        // get queues from logical device
        vkGetDeviceQueue(this->device, this->queues.graphics_family_index, 0, &this->queues.graphics_queue);
        vkGetDeviceQueue(this->device, this->queues.present_family_index,  0, &this->queues.present_queue);
        vkGetDeviceQueue(this->device, this->queues.transfer_family_index, 0, &this->queues.transfer_queue);
        vkGetDeviceQueue(this->device, this->queues.compute_family_index,  0, &this->queues.compute_queue);
    }

    //------------------------------------------------------------------------------------------------------------------------------

    bool is_device_compatible(const VkPhysicalDevice& physical_device)
    {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &device_properties);

        VkPhysicalDeviceFeatures device_features = {}; // physical device features that are supported for logical device
        vkGetPhysicalDeviceFeatures(physical_device, &device_features);

        msg::printl("Minimum uniform alignment: ", device_properties.limits.minUniformBufferOffsetAlignment);
        msg::printl("Memory allocation count: ", device_properties.limits.maxMemoryAllocationCount);
        msg::printl("Maximum descriptor image array size: ", device_properties.limits.maxPerStageDescriptorSampledImages);

        printf("Selected: %s | %s | ",device_properties.deviceName, version_to_string(device_properties.apiVersion).c_str());
        switch(device_properties.deviceType){
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   printf("VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU \n"  ); break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: printf("VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU \n"); break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            printf("VK_PHYSICAL_DEVICE_TYPE_CPU \n"           ); break;
            default:                                     printf("VK_PHYSICAL_DEVICE_TYPE_OTHER \n"         ); break;
        }

        // check device memory (not required)
        VkPhysicalDeviceMemoryProperties memory_properties = {};
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
        printf("Memory heaps: %i | types: %i \n", memory_properties.memoryHeapCount, memory_properties.memoryTypeCount);

        for(int i = 0; i < memory_properties.memoryHeapCount; i++){
            const VkMemoryHeap &heap = memory_properties.memoryHeaps[i];
            
            std::cout<< "Heap " << i << " | size: " << heap.size / (1024*1024) << "MB | flags: ";

            for(int j = 0; j < memory_properties.memoryTypeCount; j++){
                if(memory_properties.memoryTypes[j].heapIndex == i){
                    std::cout<< std::bitset<4>(memory_properties.memoryTypes[j].propertyFlags) << " ";
                }
            }
            std::cout<<std::endl;
        }

        /* VkMemoryPropertyFlagBits:
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  = 0001,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  = 0010,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 0100,
            VK_MEMORY_PROPERTY_HOST_CACHED_BIT   = 10
        */

       

       msg::printl(device_features.textureCompressionETC2, device_features.textureCompressionBC, device_features.textureCompressionASTC_LDR);

       return is_device_extensions_supported(physical_device) && device_features.samplerAnisotropy;
    }

    bool is_device_extensions_supported(const VkPhysicalDevice &physical_device)
    {
        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

        // what extensions device supports
        //for(VkExtensionProperties &properties : available_extensions) printf("%s\n", properties.extensionName);

        bool is_supported = true;
        for(const char* required_extension_name : DEVICE_EXTENSIONS){
            bool extension_found = false;

            for (const VkExtensionProperties &available_extension : available_extensions) {
                if( std::strcmp(required_extension_name, available_extension.extensionName) == 0 ) extension_found = true;
            }

            if(extension_found == false){
                printf("Extension is not supported: %s\n", required_extension_name);
                is_supported = false;
            }
        }
        return is_supported;
    }

    bool is_device_surface_capable(const VkPhysicalDevice& physical_device)
    {
        uint32_t count;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, this->surface.vulcan_surface, &this->surface.capabilities);

        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, this->surface.vulcan_surface, &count, nullptr);
        if(count == 0) throw std::runtime_error("device doesn't support any surface formats");
        std::vector<VkSurfaceFormatKHR> surface_formats(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, this->surface.vulcan_surface, &count, surface_formats.data());

        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, this->surface.vulcan_surface, &count, nullptr);
        if(count == 0) throw std::runtime_error("device doesn't support any present modes");
        std::vector<VkPresentModeKHR> present_modes(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, this->surface.vulcan_surface, &count, present_modes.data());

        // pick
        bool is_format_ok = false;
        bool is_mode_ok = false;
        bool is_image_count_ok = false;

        for (const auto& available_format : surface_formats) {
            if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                this->surface.surface_format = available_format;
                is_format_ok = true;
                break;
            }
        }
        
        /*
            VK_PRESENT_MODE_MAILBOX_KHR: When a new image is presented, it is marked as the pending image, 
            and at the next opportunity (probably after the next vertical refresh), the system will Surface it to the user. 
            If a new image is presented before this happens, that image will be shown, and the previously presented image will be discarded.
            Maximum latency
        */

        for (const auto& available_mode : present_modes) {
            if (available_mode == VK_PRESENT_MODE_MAILBOX_KHR || available_mode == VK_PRESENT_MODE_FIFO_KHR ) {
                this->surface.present_mode = available_mode;
                is_mode_ok = true;
                break;
            }
        }

        // 0 - means there's no maximum
        if(this->surface.capabilities.maxImageCount >= 3 || this->surface.capabilities.maxImageCount == 0){
            this->surface.image_count = 3;
            is_image_count_ok = true;
        }

        // depth
        this->surface.depth_format = find_depth_format(
            physical_device,
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
        
        return is_format_ok && is_mode_ok && is_image_count_ok;
    }

    VkFormat find_depth_format(const VkPhysicalDevice& physical_device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
    {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("failed to find supported depth format!");
    }

    //------------------------------------------------------------------------------------------------------------------------------

    void create_transfer_command_pool()
    {
        VkCommandPoolCreateInfo ci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        ci.flags = 0;
        ci.queueFamilyIndex = queues.transfer_family_index;

        if(vkCreateCommandPool(device, &ci, nullptr, &transfer_command_pool) == VK_SUCCESS)
        {
            printf("Created transfer command pool \n");
        }   
        else
        {
            throw std::runtime_error("failed to create command pool!");
        };
    }

};