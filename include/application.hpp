#include "common.hpp"
#include "instance.hpp"
#include "memory.hpp"
#include "pipeline.hpp"
#include "commands.hpp"
#include "model.hpp"

/*
instance class:
    instance / validation layers
    physical device / query capabilities
    device
    queues
    surface


command class (&instance)
memory management class (&instance, &command)

swapchain -> swapchain images -> imageviews -> imagebuffers -> commandbuffers




             descriptor set layout -> pipeline layout -> graphics pipeline
                        |
descriptor pool -> descriptor set -> (bind sets to command buffer)


*/
class Application{
public:
    uint32_t current_frame = 0;

    void draw(){
        // wait for fence signal (1), first frame is already signaled (behaves like debounce)
        
        vkWaitForFences(instance.device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
        vkResetFences(instance.device, 1, &in_flight_fences[current_frame]); // remove signal (0)

        uint32_t image_index;
        VkResult result;

        result = vkAcquireNextImageKHR(
            instance.device, 
            swapchain, 
            UINT64_MAX, // timeout
            image_available_semaphores[current_frame], // signal semaphore
            VK_NULL_HANDLE,
            &image_index
        );

        if(result == VK_ERROR_OUT_OF_DATE_KHR){
            printf("Swapchain changed state \n");
            //this->recreate_swapchain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        update_uniform_buffer(image_index);

        // submit command buffer to queue
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSemaphore waitSemaphores[] = {image_available_semaphores[current_frame]};
        VkSemaphore signalSemaphores[] = {render_finished_semaphores[current_frame]};

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &command_buffers[image_index];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(instance.queues.graphics_queue, 1, &submitInfo, in_flight_fences[current_frame]) != VK_SUCCESS) { 
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        // present image
        VkSwapchainKHR swapchains[] = {swapchain};
        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &image_index;

        result = vkQueuePresentKHR(instance.queues.present_queue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            //this->recreate_swapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

        //printf("Render loop %f \n", glfwGetTime());
    }

    void init_vulkan(GLFWwindow* window)
    {   
        this->instance.init(window);
        this->memory_manager.init(&this->instance);
        this->commands_manager.init(&this->instance);
        this->pipeline.init(&this->instance);

        prepare_model_buffers();
        
        create_depth_resources();
        create_swapchain();
        create_swapchain_image_views();
        create_swapchain_framebuffers();

        create_command_pool();
        
        create_texture_sampler();
        create_texture();

        create_uniform_buffers();
        create_descriptor_pool();
        create_descriptor_sets();

        create_command_buffers();
    
        create_sync_objects();
    }

    void destroy()
    {
        vkDeviceWaitIdle(instance.device);

        vkDestroyImageView(instance.device, depth_image_view, nullptr);
        vkDestroyImage(instance.device, depth_image.image, nullptr);
        vkFreeMemory(instance.device, depth_image.memory, nullptr);

        for(const VkImageView& image_view : swapchain_image_views) vkDestroyImageView(instance.device, image_view, nullptr);
        for(const VkFramebuffer& framebuffer : swapchain_framebuffers) vkDestroyFramebuffer(instance.device, framebuffer, nullptr);
        vkDestroySwapchainKHR(instance.device, swapchain, nullptr); // swapchain auto-deletes images

        vkDestroyCommandPool(instance.device, command_pool, nullptr);
        this->pipeline.destroy();
        this->instance.destroy();
    }

private:
    Instance instance;
    MemoryManager memory_manager;
    CommandsManager commands_manager;
    Pipeline pipeline; 

    Model model = Model();
    MemoryBuffer model_vertices;
    MemoryBuffer model_indices;

    MemoryImage depth_image;
    VkImageView depth_image_view;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkFramebuffer> swapchain_framebuffers;

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    std::vector<MemoryBuffer> uniform_buffers;
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

    VkSampler texture_sampler;
    MemoryImage texture_image;
    VkImageView texture_image_view;

    // semaphore for each frame
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;

    //---------------------------------------------------------------------------------

    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ci.image = image;
        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ci.format = format;
        ci.subresourceRange.aspectMask = aspectFlags; // VK_IMAGE_ASPECT_COLOR_BIT; type of data
        ci.subresourceRange.baseMipLevel = 0;
        ci.subresourceRange.levelCount = 1;
        ci.subresourceRange.layerCount = 1;
        ci.subresourceRange.baseArrayLayer = 0;

        VkImageView image_view;
        if (vkCreateImageView(instance.device, &ci, nullptr, &image_view) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return image_view;
    }

    //---------------------------------------------------------------------------------
    // command recording

    VkCommandBuffer begin_single_use_command() {
        VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; 
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = this->command_pool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(instance.device, &allocInfo, &commandBuffer);

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

        vkQueueSubmit(instance.queues.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(instance.queues.graphics_queue);

        vkFreeCommandBuffers(instance.device, command_pool, 1, &commandBuffer);
    }

    //---------------------------------------------------------------------------------
    // depth resources

    void create_depth_resources()
    {
        // {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT} - available depth formats
        VkFormat format = instance.surface.depth_format; // VK_FORMAT_D32_SFLOAT; // findDepthFormat();

        uint32_t width = instance.surface.capabilities.currentExtent.width;
        uint32_t height = instance.surface.capabilities.currentExtent.height;
        depth_image = memory_manager.create_image(width, height, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        depth_image_view = create_image_view(depth_image.image, format, VK_IMAGE_ASPECT_DEPTH_BIT);
    }


    //---------------------------------------------------------------------------------

    void create_swapchain()
    {
        VkSwapchainCreateInfoKHR ci = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        ci.surface = this->instance.surface.vulcan_surface;
        ci.minImageCount = this->instance.surface.image_count;
        ci.imageFormat = this->instance.surface.surface_format.format;
        ci.imageColorSpace = this->instance.surface.surface_format.colorSpace;
        ci.imageExtent = this->instance.surface.capabilities.currentExtent;
        ci.presentMode = this->instance.surface.present_mode;
        ci.imageArrayLayers = 1; // always 1 layer, 2 layers are for stereoscopic 3D application
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // specifies that the image can be used to create a VkImageView, use as a color
        ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; 
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // alpha channel use with other windows
        ci.clipped = VK_TRUE; // if pixels are obscured, clip them
        ci.oldSwapchain = VK_NULL_HANDLE; // VK_NULL_HANDLE or swapchain; // if window resizes, need to recreate swapchain
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // image going to be used by 1 queue
        ci.queueFamilyIndexCount = 0; // Optional
        ci.pQueueFamilyIndices = nullptr; // Optional

        // swap chain also auto-creates images
        if(vkCreateSwapchainKHR(instance.device, &ci, nullptr, &this->swapchain) == VK_SUCCESS) {
            printf("Created swapchain \n");
        }else{
            throw std::runtime_error("failed to create swap chain!");
        }

        uint32_t image_count;
        vkGetSwapchainImagesKHR(instance.device, this->swapchain, &image_count, nullptr);
        swapchain_images.resize(image_count);
        vkGetSwapchainImagesKHR(instance.device, this->swapchain, &image_count, swapchain_images.data());  
    }

    void create_swapchain_image_views()
    {
        this->swapchain_image_views.resize(this->swapchain_images.size());
        for(int i = 0; i < this->swapchain_images.size(); i++){
            // alternative format location - 
            this->swapchain_image_views[i] = create_image_view(this->swapchain_images[i], instance.surface.surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void create_swapchain_framebuffers() // Renderpass use framebuffers
    {
        this->swapchain_framebuffers.resize(this->swapchain_image_views.size());

        for (size_t i = 0; i < this->swapchain_image_views.size(); i++) 
        {
            std::array<VkImageView, 2> attachments = { this->swapchain_image_views[i], this->depth_image_view };

            VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
            framebufferInfo.renderPass = this->pipeline.render_pass;
            framebufferInfo.attachmentCount = (uint32_t)attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = this->instance.surface.capabilities.currentExtent.width;
            framebufferInfo.height = this->instance.surface.capabilities.currentExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(instance.device, &framebufferInfo, nullptr, &swapchain_framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }

        printf("Created framebuffers \n");
    }

    //---------------------------------------------------------------------------------

    void create_command_pool()
    {
        VkCommandPoolCreateInfo ci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        ci.flags = 0;
        ci.queueFamilyIndex = instance.queues.graphics_family_index;

        if(vkCreateCommandPool(instance.device, &ci, nullptr, &this->command_pool) == VK_SUCCESS)
        {
            printf("Created command pool \n");
        }   
        else
        {
            throw std::runtime_error("failed to create command pool!");
        };
    }

    void create_command_buffers()
    {
        command_buffers.resize(swapchain_images.size());

        VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        alloc_info.commandPool = this->command_pool;
        alloc_info.commandBufferCount = (uint32_t)command_buffers.size();
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        if(vkAllocateCommandBuffers(instance.device, &alloc_info, command_buffers.data()) != VK_SUCCESS){
            throw std::runtime_error("failed to allocate command buffers!");
        };

        std::array<VkClearValue, 2> clear_values = {};
        clear_values[0].color = {0.0, 0.0, 0.0, 1.0};
        clear_values[1].depthStencil = {1.0, 0};

        for(int i = 0; i < command_buffers.size(); i++)
        {
            // render pass info for recodring
            VkRenderPassBeginInfo render_pass_bi = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
            render_pass_bi.renderPass = this->pipeline.render_pass;
            render_pass_bi.framebuffer = this->swapchain_framebuffers[i]; // framebuffer for each swap chain image that specifies it as color attachment.
            render_pass_bi.renderArea.offset = {0, 0}; // render area
            render_pass_bi.renderArea.extent = this->instance.surface.capabilities.currentExtent;
            render_pass_bi.clearValueCount = (uint32_t)clear_values.size();
            render_pass_bi.pClearValues = clear_values.data();

            VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            vkBeginCommandBuffer(command_buffers[i], &begin_info);
            
            // RECORD START

            vkCmdBeginRenderPass(command_buffers[i], &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);
            // VK_SUBPASS_CONTENTS_INLINE - render pass commands will be embedded in the primary command buffer itself, no secondary buffers.
            // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS - The render pass commands will be executed from secondary command buffers.

            vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.graphics_pipeline);
            // VK_PIPELINE_BIND_POINT_GRAPHICS - specify if graphics or compute pipeline

            VkBuffer vertex_buffers[] = {model_vertices.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(command_buffers[i], model_indices.buffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline_layout, 0, 1, &descriptor_sets[i], 0, nullptr);
            //vkCmdDraw(command_buffers[i], (uint32_t)model.vertices.size(), 1, 0, 0);
            vkCmdDrawIndexed(command_buffers[i], (uint32_t)model.indices.size(), 1, 0, 0, 0);

            vkCmdEndRenderPass(command_buffers[i]);

            // RECORD END

            if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        printf("recorded commands \n");
    }

    void prepare_model_buffers()
    {
        VkDeviceSize size;

        size = sizeof(Vertex) * model.vertices.size();
        model_vertices = memory_manager.create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        memory_manager.fill_memory_buffer(model_vertices, model.vertices.data(), size);

        size = sizeof(uint16_t) * model.indices.size();
        model_indices = memory_manager.create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        memory_manager.fill_memory_buffer(model_indices, model.indices.data(), size);

        printf("Vertex and index buffers created \n");
    }

    void create_uniform_buffers(){

        VkDeviceSize size = sizeof(UniformBufferObject);
        uniform_buffers.resize(swapchain_images.size());

        for (size_t i = 0; i < swapchain_images.size(); i++) 
        {
            uniform_buffers[i] = memory_manager.create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }

        printf("created uniform buffers \n");
    }

    
    void update_uniform_buffer(uint32_t currentImage)
    {
        float width = instance.surface.capabilities.currentExtent.width;
        float height = instance.surface.capabilities.currentExtent.height;

        UniformBufferObject ubo = {};
        ubo.model = glm::mat4(1.0);
        ubo.view = glm::mat4(1.0);
        ubo.proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);
        ubo.proj[1][1] *= -1;

        float angle = glfwGetTime()*15;
        float distance = 2;
        float x = glm::sin( glm::radians(angle) ) * distance;
        float y = glm::cos( glm::radians(angle) ) * distance;

        ubo.view = glm::lookAt(glm::vec3(x,y,2), glm::vec3(0,0,0), glm::vec3(0,0,1));
        ubo.model = glm::translate(ubo.model, glm::vec3(0,0,.5));

        void* data;
        vkMapMemory(instance.device, uniform_buffers[currentImage].memory, 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(instance.device, uniform_buffers[currentImage].memory);
    }
    

    void create_descriptor_pool(){
        std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[0].descriptorCount = (uint32_t)swapchain_images.size();
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[1].descriptorCount = (uint32_t)swapchain_images.size();

        VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount = (uint32_t)pool_sizes.size();
        poolInfo.pPoolSizes = pool_sizes.data();
        poolInfo.maxSets = (uint32_t)swapchain_images.size();

        if (vkCreateDescriptorPool(instance.device, &poolInfo, nullptr, &this->descriptor_pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

        printf("created descriptor pool \n");
    }

    void create_descriptor_sets(){
        std::vector<VkDescriptorSetLayout> layouts(swapchain_images.size(), this->pipeline.descriptor_set_layout);

        VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = this->descriptor_pool;
        allocInfo.descriptorSetCount = (uint32_t)swapchain_images.size();
        allocInfo.pSetLayouts = layouts.data();

        descriptor_sets.resize(swapchain_images.size());
        if (vkAllocateDescriptorSets(instance.device, &allocInfo, descriptor_sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        printf("Allocated descriptor sets \n");

        for (size_t i = 0; i < swapchain_images.size(); i++) {

            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = uniform_buffers[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture_image_view;
            imageInfo.sampler = texture_sampler;
            

            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptor_sets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptor_sets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;
            
            vkUpdateDescriptorSets(instance.device, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

        }

        printf("Descriptor sets ready \n");
    }

    void create_texture()
    {
        // read image
        int width, height, channel;
        stbi_uc* pixels = stbi_load("textures/image.jpg", &width, &height, &channel, STBI_rgb_alpha);
        if(!pixels) throw std::runtime_error("failed to load texture image!");
        VkDeviceSize memorySize = width * height *4;

        this->texture_image = memory_manager.create_image(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        transitionImageLayout(texture_image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        transitionImageLayout(texture_image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        this->texture_image_view = create_image_view(texture_image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        //memory_manager.fill_memory_image(this->texture_image, pixels, memorySize);
        stbi_image_free(pixels);

        printf("created texture \n");
    }

    void create_texture_sampler()
    {
        VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
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

        if (vkCreateSampler(instance.device, &samplerInfo, nullptr, &texture_sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }

        printf("created sampler \n");
    }

    void create_sync_objects()
    {
        // semaphores are designed to be used to sync queues, GPU-GPU
        // fences are designed to sync vulkan, GPU-CPU
        image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            if (vkCreateSemaphore(instance.device, &semaphoreInfo, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(instance.device, &semaphoreInfo, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
                vkCreateFence(instance.device, &fenceInfo, nullptr, &in_flight_fences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores!");
            }
        }

        printf("Created sync objects \n");
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {

        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
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
        /*
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        */
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        //-------------------------------------------------------------

        VkCommandBuffer commandBuffer = begin_single_use_command();

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        end_single_use_command(commandBuffer);
    }
};
