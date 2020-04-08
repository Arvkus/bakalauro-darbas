```cpp
#include "common.hpp"
#include "instance.hpp"
#include "display.hpp"

/*
instance class:
    instance / validation layers
    physical device / query capabilities
    device
    queues
    surface


command class (&instance)
memory management class (&instance, &command)

swapchain - swapchain images - imageviews





*/
class Application{
public:
    void draw(){
        //printf("Render loop %f \n", glfwGetTime());
    }

    void init_vulkan(GLFWwindow* window)
    {
        this->instance.init(window);
    }

    void destroy(){
        /*
        vkDeviceWaitIdle(this->device.logical_device);

        vkDestroyPipeline(this->device.logical_device, this->graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(this->device.logical_device, this->pipeline_layout, nullptr);
        vkDestroyRenderPass(this->device.logical_device, this->render_pass, nullptr);

        vkDestroySwapchainKHR(this->device.logical_device, this->display.swapchain, nullptr);
        vkDestroySurfaceKHR(this->instance, this->surface, nullptr);

        vkDestroyDevice(this->device.logical_device, nullptr);
        vkDestroyInstance(this->instance, nullptr);
        glfwDestroyWindow(this->window);
        */
        glfwTerminate();
    }

private:

    Instance instance;


    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;


    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapshain_image_views;

    VkRenderPass render_pass;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkCommandPool command_pool;


    void prepare_device()
    {
        device.pick_physical_device(this->instance);
        device.pick_device_queues(this->surface);
        device.create_logical_device();
    }

  
    void create_swapchain()
    {
        display.query_capabilities(this->device.physical_device, this->surface);
        display.create_swapchain(this->device.logical_device, this->surface);

        vkGetSwapchainImagesKHR(this->device.logical_device, this->display.swapchain, &this->display.image_count, nullptr); 
        swapchain_images.resize(this->display.image_count);
        vkGetSwapchainImagesKHR(this->device.logical_device, this->display.swapchain, &this->display.image_count, swapchain_images.data());    
    }
 
    


    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.image = image;
        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ci.format = format; // VK_FORMAT_R8G8B8A8_SRGB;
        ci.subresourceRange.aspectMask = aspectFlags; // VK_IMAGE_ASPECT_COLOR_BIT; type of data
        ci.subresourceRange.baseMipLevel = 0;
        ci.subresourceRange.levelCount = 1;
        ci.subresourceRange.baseArrayLayer = 0;

        VkImageView image_view;
        if (vkCreateImageView(this->device.logical_device, &ci, nullptr, &image_view) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return image_view;
    }

    void create_render_pass()
    {
        /*
            Render pass is set of attachments, dependencies, subpasses
            Attachment - describe how content is treated at the start/end of each render pass instance, including format, sample count
            Subpass - render commands are recorded into supbass
            Dependency - describe execution and memory dependencies between subpasses.

            Attachment types: output (color, depth/stencil), input, resolve (no read/write, preserve content)

            The actual images to be used are provided when the render pass is used, via the VkFrameBuffer
            Render pass is wrapped into framebuffer
        */

        VkAttachmentDescription color_attachment = {};
        color_attachment.format = VK_FORMAT_R8G8B8_SRGB; // capabilities.format
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // what to do with data before render
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // what to do with data after render
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depth_attachment = {};
        depth_attachment.format = VK_FORMAT_D32_SFLOAT; // findDepthFormat();
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        //------------------------------------------

        VkAttachmentReference color_attachment_ref = {};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_attachment_ref = {};
        depth_attachment_ref.attachment = 1;
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;
        subpass.pDepthStencilAttachment = &depth_attachment_ref;

        //------------------------------------------

        // specify memory and execution dependencies between subpasses
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // on what stage dependant
        dependency.srcAccessMask = 0; 

        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // where to have things on this pass
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        //------------------------------------------

        std::array<VkAttachmentDescription, 2> attachments = {
            color_attachment,
            depth_attachment,
        };

        VkRenderPassCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        ci.attachmentCount = (uint32_t)attachments.size();
        ci.pAttachments = attachments.data();
        ci.subpassCount = 1;
        ci.pSubpasses = &subpass;
        ci.dependencyCount = 1;
        ci.pDependencies = &dependency;

        if(vkCreateRenderPass(this->device.logical_device, &ci, nullptr, &render_pass) == VK_SUCCESS){
            printf("Created render pass \n");
        }else{
            throw std::runtime_error("failed to create render pass");
        };
    }

    void create_descriptor_set_layout()
    {
        // define what type resources to use

        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

        VkDescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = (uint32_t)bindings.size();
        createInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(this->device.logical_device, &createInfo, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    VkShaderModule create_shader_module(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(this->device.logical_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }


    void create_graphics_pipeline()
    {
        // dynamic things in pipeline: size of the viewport, line width and blend constants
        // stages - https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPipelineStageFlagBits.html

        // input assembler
        // vertex shader
        // tessallation shader
        // geometry shader
        // rasteriation
        // fragment shader
        // color blending

        std::vector<char> vertShaderCode = read_file("shaders/vert.spv");
        std::vector<char> fragShaderCode = read_file("shaders/frag.spv");

        // wrap code in shader module to pass it into pipeline
        VkShaderModule vertShaderModule = create_shader_module(vertShaderCode);
        VkShaderModule fragShaderModule = create_shader_module(fragShaderCode);

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
        auto bindingDescription = Vertex::get_binding_description();
        auto attributeDescriptions = Vertex::get_attribute_descriptions();

        // vertex inut
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

        // viewport - shrink image, scissor - crop image
        // (0, 0) to (width, height)
        VkViewport viewport = {}; 
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)this->display.capabilities.currentExtent.width;
        viewport.height = (float)this->display.capabilities.currentExtent.width;
        viewport.minDepth = 0.0f; // depth values for framebuffer
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {}; 
        scissor.offset = {0, 0};
        scissor.extent = this->display.capabilities.currentExtent;

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

        // what descriptor set layouts to use
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {}; 
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &this->descriptor_set_layout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(this->device.logical_device, &pipelineLayoutInfo, nullptr, &this->pipeline_layout) != VK_SUCCESS) {
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
        pipelineInfo.layout = pipeline_layout;
        pipelineInfo.renderPass = render_pass;
        pipelineInfo.subpass = 0;

        // required for switching between multiple pipelines
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(this->device.logical_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }else{
            std::cout<< "Successfully created graphics pipelines" << std::endl;
        }

        vkDestroyShaderModule(this->device.logical_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(this->device.logical_device, vertShaderModule, nullptr);
    }



    //---------------------------------------------------------------------------------
    // command recording

    VkCommandBuffer begin_single_use_command() {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = this->command_pool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(this->device.logical_device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void end_single_use_command(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(this->device.queues.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(this->device.queues.graphics_queue);

        vkFreeCommandBuffers(this->device.logical_device, command_pool, 1, &commandBuffer);
    }

    //---------------------------------------------------------------------------------

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory ){

        VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.size = size;
        createInfo.usage = usage;

        if(vkCreateBuffer(this->device.logical_device, &createInfo, nullptr, &buffer) != VK_SUCCESS){
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements = {};
        vkGetBufferMemoryRequirements(this->device.logical_device, buffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

        if(vkAllocateMemory(this->device.logical_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS){
            throw std::runtime_error("failed to allocate memory!");
        };

        vkBindBufferMemory(this->device.logical_device, buffer, bufferMemory, 0);
    }

    uint32_t find_memory_type(uint32_t typeBits, VkMemoryPropertyFlags requiredProperties) {

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(this->device.physical_device, &memProperties);

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

    //---------------------------------------------------------------------------------


};
