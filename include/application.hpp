#include "common.hpp"
#include "instance.hpp"
#include "memory.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include "descriptors.hpp"
#include "model.hpp"
#include "input.hpp"
#include "camera.hpp"
#include "loader.hpp"

class Application{
public:

    void draw()
    {
        if(is_model_update()) return;  // do not render while model is loading
        if(RECREATE_SWAPCHAIN) return; // do not render while swapchain is recreating

        std::optional<uint32_t> next_image = swapchain.accquire_next_image();
        if(!next_image.has_value()) RECREATE_SWAPCHAIN = true; // recreate_swapchain();

        update_uniform_buffer(next_image.value());
        
        bool is_presented = swapchain.present_image(next_image.value());
        if(!is_presented) RECREATE_SWAPCHAIN = true; // recreate_swapchain();
    }

    Model model;
    Model skybox;

    void init_vulkan(GLFWwindow* window)
    {   
        this->instance.init(window);
        this->descriptors.init(&this->instance);

        this->render_pass = create_render_pass(&this->instance);
        this->pipeline.init(&this->instance, &this->descriptors, &this->render_pass);
        this->pipeline.create_graphics_pipeline();

        this->skybox_pipeline.init(&this->instance, &this->descriptors, &this->render_pass);
        this->skybox_pipeline.create_skybox_pipeline();

        this->swapchain.init(&this->instance, &this->render_pass);
        
        Loader loader = Loader();
        
        skybox = loader.load("models/cube.glb");
        skybox.prepare_model(&this->instance, &this->descriptors);
        
        model = loader.load("models/tests/glTF/Fish/BarramundiFish.gltf");
        model.prepare_model(&this->instance, &this->descriptors);

        camera.set_region(model.get_region());

        create_enviroment_buffer();
        this->descriptors.bind_enviroment_image(&this->enviroment_image);
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
        this->pipeline.destroy();
        this->skybox_pipeline.destroy();
        vkDestroyCommandPool(instance.device, command_pool, nullptr);

        // recreate objects
        this->pipeline.init(&this->instance, &this->descriptors, &this->render_pass);
        this->pipeline.create_graphics_pipeline();

        this->skybox_pipeline.init(&this->instance, &this->descriptors, &this->render_pass);
        this->skybox_pipeline.create_skybox_pipeline();

        this->swapchain.init(&this->instance, &this->render_pass);

        create_command_pool();
        create_command_buffers();

        this->swapchain.bind_command_buffers(this->command_buffers.data());
    }

    void destroy()
    {
        vkDeviceWaitIdle(instance.device);

        this->swapchain.destroy();
        this->descriptors.destroy();
        
        this->skybox_pipeline.destroy();
        this->pipeline.destroy();

        enviroment_image.destroy();
        skybox.destroy();
        model.destroy();

        vkDestroyRenderPass(instance.device, this->render_pass, nullptr);
        vkDestroyCommandPool(instance.device, command_pool, nullptr);

        this->instance.destroy();
    }

private:
    Instance instance;
    Pipeline pipeline; 
    Pipeline skybox_pipeline;
    Descriptors descriptors;
    Swapchain swapchain;
    VkRenderPass render_pass;

    Camera camera = Camera();

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    Image enviroment_image;

    //---------------------------------------------------------------------------------

    bool is_model_update()
    {
        if(Input::Keys::L == false) return false;

        pfd::open_file f = pfd::open_file("Choose files to read", FILE_PATH, { "Model Files (.glb .gltf)", "*.glb *.gltf"}, false);
        if(f.result().size() > 0){
             vkDeviceWaitIdle(instance.device);
            msg::printl("File selected from dialog: ", f.result()[0]);

            vkDestroyCommandPool(instance.device, command_pool, nullptr);
            this->descriptors.destroy();
            this->model.destroy();
            //-----------------------------------------
            this->descriptors.init(&this->instance);
            this->descriptors.bind_enviroment_image(&this->enviroment_image);
            
            Loader loader = Loader();
            model = loader.load(f.result()[0].c_str());
            model.prepare_model(&this->instance, &this->descriptors);

            camera.set_region(model.get_region());

            this->descriptors.create_descriptor_sets();
            create_command_pool();
            create_command_buffers();

        }else{
            msg::printl("No file selected from dialog");
        }
        return false;
    }
    

    //---------------------------------------------------------------------------------
    float exposure = 0.3;
    void update_uniform_buffer(uint32_t current_image)
    {
        camera.move();
        Input::reset();

        //std::cout<< "A: " <<std::boolalpha <<Input::Keys::A << " | W: " << Input::Keys::W <<std::endl;
 
        float width = instance.surface.capabilities.currentExtent.width;
        float height = instance.surface.capabilities.currentExtent.height;

        UniformCameraStruct ubo = {};
        ubo.view = camera.cframe(); //glm::translate(glm::mat4(1.0), glm::vec3(0,0,-20));
        ubo.proj = glm::perspective(glm::radians(45.0f), width / height, 0.02f, 1000.0f);

        UniformPropertiesStruct properties; // ...

        /*
        if(Input::Keys::Equal) this->exposure += this->exposure/20; ubo.exposure = this->exposure;
        if(Input::Keys::Minus) this->exposure -= this->exposure/20; ubo.exposure = this->exposure;
        //ubo.model = glm::translate(ubo.model, glm::vec3(0,0,.5));
        */

        descriptors.view_buffer.fill_memory(&ubo, sizeof(ubo));
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
            render_pass_bi.renderPass = this->render_pass;
            render_pass_bi.framebuffer = this->swapchain.swapchain_framebuffers[i]; // framebuffer for each swap chain image that specifies it as color attachment.
            render_pass_bi.renderArea.offset = {0, 0}; // render area
            render_pass_bi.renderArea.extent = this->instance.surface.capabilities.currentExtent;
            render_pass_bi.clearValueCount = (uint32_t)clear_values.size();
            render_pass_bi.pClearValues = clear_values.data();

            VkDeviceSize offsets[1] = {0};

            uint32_t min_align = 256;
            uint32_t dyn_align = sizeof(float);
            dyn_align = (dyn_align + min_align - 1) & ~(min_align - 1);

            

            VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            vkBeginCommandBuffer(command_buffers[i], &begin_info);
            
            
            // RECORD START

            vkCmdBeginRenderPass(command_buffers[i], &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);
            // VK_SUBPASS_CONTENTS_INLINE - render pass commands will be embedded in the primary command buffer itself, no secondary buffers.
            // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS - The render pass commands will be executed from secondary command buffers.
            //------------------------------------------
            
            vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->skybox_pipeline.graphics_pipeline);
            skybox.draw(&command_buffers[i], &pipeline.pipeline_layout, &descriptors);
            
            //------------------------------------------
            
            vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.graphics_pipeline);
            model.draw(&command_buffers[i], &pipeline.pipeline_layout, &descriptors);
            
            //------------------------------------------
            vkCmdEndRenderPass(command_buffers[i]);

            // RECORD END

            if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        printf("Recorded commands \n");
    }

    void create_enviroment_buffer()
    {
        VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
        int width = 0, height = 0, channel = 0;
        float* pixels = stbi_loadf("textures/pond.hdr", &width, &height, &channel, STBI_rgb_alpha);
        if(!pixels) throw std::runtime_error("failed to load texture image!");

        // for (int i = 0; i < width*height; i++) pixels[i] *=255; 

        this->enviroment_image.init(&this->instance);
        this->enviroment_image.create_image(width, height, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        this->enviroment_image.fill_memory(width, height, 4*sizeof(float), pixels);
        this->enviroment_image.create_image_view(format, VK_IMAGE_ASPECT_COLOR_BIT);
        stbi_image_free(pixels);
        
        msg::printl("enviroment created");
    }  

    void create_tonemapped_enviroment_buffer()
    {
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
        int width = 0, height = 0, channel = 0;
        stbi_uc* pixels = stbi_load("textures/saturn_hdr.png", &width, &height, &channel, STBI_rgb_alpha);
        if(!pixels) throw std::runtime_error("failed to load texture image!");


        this->enviroment_image.init(&this->instance);
        this->enviroment_image.create_image(width, height, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        this->enviroment_image.fill_memory(width, height, 4, pixels);
        this->enviroment_image.create_image_view(format, VK_IMAGE_ASPECT_COLOR_BIT);
        stbi_image_free(pixels);
        
        msg::printl("enviroment created");
    }    
};

// https://hdrihaven.com/hdri/?c=night&h=pond_bridge_night
//https://github.com/SaschaWillems/Vulkan/blob/master/examples/texturecubemap/texturecubemap.cpp