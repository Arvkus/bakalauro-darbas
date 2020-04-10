#include "common.hpp"
#include "instance.hpp"
#include "memory.hpp"
#include "pipeline.hpp"


class Swapchain
{
public:

    void init(Instance *instance, Pipeline *pipeline){
        this->instance = instance;
        this->pipeline = pipeline;

        create_depth_resources();
        create_swapchain();
        create_swapchain_image_views();
        create_swapchain_framebuffers();
        create_sync_objects();
    }

    void destroy(){
        vkDestroySwapchainKHR(instance->device, vulkan_swapchain, nullptr);

        for(const VkFramebuffer& framebuffer : swapchain_framebuffers) vkDestroyFramebuffer(instance->device, framebuffer, nullptr);
        for(const VkImageView& image_view : swapchain_image_views) vkDestroyImageView(instance->device, image_view, nullptr);
        depth_image.destroy();

        for(const VkFence& fence : in_flight_fences) vkDestroyFence(instance->device, fence, nullptr);
        for(const VkSemaphore& semaphore : image_available_semaphores) vkDestroySemaphore(instance->device, semaphore, nullptr);
        for(const VkSemaphore& semaphore : render_finished_semaphores) vkDestroySemaphore(instance->device, semaphore, nullptr);
    }

    Image depth_image;
    VkSwapchainKHR vulkan_swapchain;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkFramebuffer> swapchain_framebuffers;

    // semaphore for each frame
    uint32_t current_frame = 0;
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;

    //------------------------------------------------------------

    void create_depth_resources()
    {
        // {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT} - available depth formats
        VkFormat format = instance->surface.depth_format; // VK_FORMAT_D32_SFLOAT; // findDepthFormat();
        uint32_t width = instance->surface.capabilities.currentExtent.width;
        uint32_t height = instance->surface.capabilities.currentExtent.height;

        depth_image.init(instance);
        depth_image.create_image(width, height, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        depth_image.create_image_view(format, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    void create_swapchain()
    {
        VkSwapchainCreateInfoKHR ci = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        ci.surface = instance->surface.vulcan_surface;
        ci.minImageCount = instance->surface.image_count;
        ci.imageFormat = instance->surface.surface_format.format;
        ci.imageColorSpace = instance->surface.surface_format.colorSpace;
        ci.imageExtent = instance->surface.capabilities.currentExtent;
        ci.presentMode = instance->surface.present_mode;
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
        if(vkCreateSwapchainKHR(instance->device, &ci, nullptr, &this->vulkan_swapchain) == VK_SUCCESS) {
            printf("Created swapchain \n");
        }else{
            throw std::runtime_error("failed to create swap chain!");
        }

        uint32_t image_count;
        vkGetSwapchainImagesKHR(instance->device, this->vulkan_swapchain, &image_count, nullptr);
        swapchain_images.resize(image_count);
        vkGetSwapchainImagesKHR(instance->device, this->vulkan_swapchain, &image_count, swapchain_images.data());  
    }

    void create_swapchain_image_views()
    {
        this->swapchain_image_views.resize(this->swapchain_images.size());
        for(int i = 0; i < this->swapchain_images.size(); i++)
        {
            VkImageViewCreateInfo ci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            ci.image = swapchain_images[i];
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = instance->surface.surface_format.format;
            ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ci.subresourceRange.baseMipLevel = 0;
            ci.subresourceRange.levelCount = 1;
            ci.subresourceRange.baseArrayLayer = 0;
            ci.subresourceRange.layerCount = 1;

            if (vkCreateImageView(instance->device, &ci, nullptr, &swapchain_image_views[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture image view!");
            }
        }
    }

    void create_swapchain_framebuffers() // Renderpass use framebuffers
    {
        this->swapchain_framebuffers.resize(this->swapchain_images.size());

        for (size_t i = 0; i < this->swapchain_images.size(); i++) 
        {
            std::array<VkImageView, 2> attachments = { this->swapchain_image_views[i], this->depth_image.image_view };

            VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
            framebufferInfo.renderPass = pipeline->render_pass;
            framebufferInfo.attachmentCount = (uint32_t)attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = instance->surface.capabilities.currentExtent.width;
            framebufferInfo.height = instance->surface.capabilities.currentExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(instance->device, &framebufferInfo, nullptr, &swapchain_framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }

        printf("Created framebuffers \n");
    }

    //------------------------------------------------------------

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

            if (vkCreateSemaphore(instance->device, &semaphoreInfo, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(instance->device, &semaphoreInfo, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
                vkCreateFence(instance->device, &fenceInfo, nullptr, &in_flight_fences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores!");
            }
        }

        printf("Created sync objects \n");
    }

    //------------------------------------------------------------

    uint32_t accquire_next_image(){
        // wait for fence signal (1), first frame is already signaled (behaves like debounce)
        
        vkWaitForFences(instance->device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
        vkResetFences(instance->device, 1, &in_flight_fences[current_frame]); // remove signal (0)

        uint32_t image_index;
        VkResult result = vkAcquireNextImageKHR(
            instance->device, 
            vulkan_swapchain, 
            UINT64_MAX, // timeout
            image_available_semaphores[current_frame], // signal semaphore
            VK_NULL_HANDLE,
            &image_index
        );

        if(result == VK_ERROR_OUT_OF_DATE_KHR){
            printf("Swapchain changed state \n");
            //this->recreate_swapchain();
            return image_index;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        return image_index;
    }

    void present_image(uint32_t image_index){
        //printf("frame: %i | image: %i \n", current_frame, image_index);
        //std::cin.get();

        // submit command buffer to queue
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSemaphore waitSemaphores[] = {image_available_semaphores[current_frame]};
        VkSemaphore signalSemaphores[] = {render_finished_semaphores[current_frame]};

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = (command_buffers + image_index);
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(instance->queues.graphics_queue, 1, &submitInfo, in_flight_fences[current_frame]) != VK_SUCCESS) { 
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        // present image
        VkSwapchainKHR swapchains[] = {vulkan_swapchain};
        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &image_index;

        VkResult result = vkQueuePresentKHR(instance->queues.present_queue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            //this->recreate_swapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
        //printf("Render loop %f \n", glfwGetTime());
    }


    void bind_command_buffers(VkCommandBuffer *source){
        this->command_buffers = source;
    }

private:
    Instance *instance;
    Pipeline *pipeline;
    VkCommandBuffer *command_buffers;
};