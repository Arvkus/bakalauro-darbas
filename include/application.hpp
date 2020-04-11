#include "common.hpp"
#include "instance.hpp"
#include "memory.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include "descriptors.hpp"
#include "model.hpp"
#include "input.hpp"
#include "camera.hpp"

// + mouse input
// + uniform update based on input
// - prepare pbr images .. https://www.khronos.org/blog/art-pipeline-for-gltf
// + delete objects on exit
// + swapchain recreation
// + download 12 years a slave

class Application{
public:

    void draw()
    {
        if(RECREATE_SWAPCHAIN) return; // do not render while swapchain is recreating

        std::optional<uint32_t> next_image = swapchain.accquire_next_image();
        if(!next_image.has_value()) RECREATE_SWAPCHAIN = true; // recreate_swapchain();

        update_uniform_buffer(next_image.value());
        
        bool is_presented = swapchain.present_image(next_image.value());
        if(!is_presented) RECREATE_SWAPCHAIN = true; // recreate_swapchain();
    }

    void init_vulkan(GLFWwindow* window)
    {   
        this->instance.init(window);
        this->descriptors.init(&this->instance);
        this->pipeline.init(&this->instance, &this->descriptors);
        this->swapchain.init(&this->instance, &this->pipeline);
        
        create_vertex_buffers();
        create_image_buffers();

        this->descriptors.bind_diffuse_image(&this->texture_image);
        this->descriptors.create_descriptor_sets();

        create_command_pool();
        create_command_buffers();

        this->swapchain.bind_command_buffers(this->command_buffers.data());
    }

    void recreate_swapchain()
    {
        instance.update_surface_capabilities();
        vkDeviceWaitIdle(instance.device);

        printf("---------------------\n");
        
        // destroy outdated objects
        this->swapchain.destroy();
        this->descriptors.destroy();
        this->pipeline.destroy();
        vkDestroyCommandPool(instance.device, command_pool, nullptr);

        // recreate objects
        
        this->descriptors.init(&this->instance);
        this->pipeline.init(&this->instance, &this->descriptors);
        this->swapchain.init(&this->instance, &this->pipeline);

        this->descriptors.bind_diffuse_image(&this->texture_image);
        this->descriptors.create_descriptor_sets();

        create_command_pool();
        create_command_buffers();

        this->swapchain.bind_command_buffers(this->command_buffers.data());
    }

    void destroy()
    {
        vkDeviceWaitIdle(instance.device);

        this->swapchain.destroy();
        this->descriptors.destroy();
        this->pipeline.destroy();

        model_vertices.destroy();
        model_indices.destroy();
        texture_image.destroy();

        vkDestroyCommandPool(instance.device, command_pool, nullptr);

        this->instance.destroy();
    }

private:
    Instance instance;
    Pipeline pipeline; 
    Descriptors descriptors;
    Swapchain swapchain;

    Camera camera = Camera();
    Model model = Model();
    Buffer model_vertices;
    Buffer model_indices;

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    Image texture_image;

    //---------------------------------------------------------------------------------

    void update_uniform_buffer(uint32_t current_image)
    {
        camera.move();
        Input::reset();

        //std::cout<< "A: " <<std::boolalpha <<Input::Keys::A << " | W: " << Input::Keys::W <<std::endl;
 
        float width = instance.surface.capabilities.currentExtent.width;
        float height = instance.surface.capabilities.currentExtent.height;

        UniformBufferObject ubo = {};
        ubo.model = glm::mat4(1.0);
        ubo.view = camera.cframe(); 
        ubo.proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);
        //ubo.model = glm::translate(ubo.model, glm::vec3(0,0,.5));

        descriptors.uniform_buffers[current_image].fill_memory(&ubo, sizeof(ubo));
    }

    //---------------------------------------------------------------------------------

    void create_command_pool()
    {
        VkCommandPoolCreateInfo ci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        ci.flags = 0;
        ci.queueFamilyIndex = instance.queues.graphics_family_index;

        if(vkCreateCommandPool(instance.device, &ci, nullptr, &this->command_pool) != VK_SUCCESS){
            throw std::runtime_error("failed to create command pool!");
        };
    }

    void create_command_buffers()
    {
        command_buffers.resize(instance.surface.image_count);

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
            render_pass_bi.framebuffer = this->swapchain.swapchain_framebuffers[i]; // framebuffer for each swap chain image that specifies it as color attachment.
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

            vkCmdBindDescriptorSets(
                command_buffers[i], 
                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                pipeline.pipeline_layout, 0, 1, 
                &descriptors.descriptor_sets[i], 
                0, nullptr
            );

            //vkCmdDraw(command_buffers[i], (uint32_t)model.vertices.size(), 1, 0, 0);
            vkCmdDrawIndexed(command_buffers[i], (uint32_t)model.indices.size(), 1, 0, 0, 0);

            vkCmdEndRenderPass(command_buffers[i]);

            // RECORD END

            if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        printf("Recorded commands \n");
    }

    void create_vertex_buffers()
    {
        VkDeviceSize size;

        size = sizeof(Vertex) * model.vertices.size();
        model_vertices.init(&this->instance);
        model_vertices.create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        model_vertices.fill_memory(model.vertices.data(), size);

        size = sizeof(uint16_t) * model.indices.size();
        model_indices.init(&this->instance);
        model_indices.create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        model_indices.fill_memory(model.indices.data(), size);

        printf("Vertex and index buffers created \n");
    }

    void create_image_buffers()
    {
        // read image
        int width, height, channel;
        stbi_uc* pixels = stbi_load("textures/image.jpg", &width, &height, &channel, STBI_rgb_alpha);
        if(!pixels) throw std::runtime_error("failed to load texture image!");
        VkDeviceSize memorySize = width * height *4;

        this->texture_image.init(&this->instance);
        this->texture_image.create_image(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        this->texture_image.fill_memory(width, height, pixels);
        this->texture_image.create_image_view(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        stbi_image_free(pixels);

        printf("Created texture \n");
    }
};
