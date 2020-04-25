#include "common.hpp"
#include "application.hpp"
#include "input.hpp"
#include "loader.hpp"

Application app;
GLFWwindow *window;

class Animal{
public:
    std::string name = "hohho";
    virtual void sound(){
        msg::printl("angry animal sounds");
    }
};

class Dog : public Animal{
public:
    int age = 34;
    void sound(){
        msg::printl("bark");
    }
};

int main2(int argc, char *argv[])
{

    std::vector<std::unique_ptr<Animal>> animals;
    std::unique_ptr<Animal> anim(new Animal());


    animals.push_back(std::move(anim));
    animals.push_back(std::unique_ptr<Dog>(new Dog()));

 
    
    for(std::unique_ptr<Animal>& a : animals){
        a->sound();
    }
    

    /*
    std::vector<std::unique_ptr<Animal>> animals;


    animals.push_back(std::unique_ptr<Animal>(new Animal()));
    animals.push_back(std::unique_ptr<Dog>(new Dog()));

    
    for(std::unique_ptr<Animal>& a : animals){
        a->sound();
    }
    */
    
    

    /*
    int width = 0, height = 0, channel = 0;
    stbi_uc* pixels = stbi_load("textures/image.jpg",&width, &height, &channel, STBI_rgb_alpha); 
    msg::printl(width, " ",height, " ", channel, " ");

    stbi_uc* output = (stbi_uc*) malloc(MAX_IMAGE_SIZE * MAX_IMAGE_SIZE * channel);
    stbir_resize_uint8(pixels, width , height , 0,
        output, MAX_IMAGE_SIZE, MAX_IMAGE_SIZE, 0, channel);
         stbi_image_free(pixels);
    */


   
    std::cin.get();
    return 0;
}

void render_thread_function()
{
    uint32_t frame_count = 0;
    double time_begin = glfwGetTime();
    int width = 0, height = 0; // window minimize case

    try
    {
        while (APP_RUNNING)
        {
            glfwGetFramebufferSize(window, &width, &height);
            if (width != 0 && height != 0) app.draw(); // draw only if window has size
            std::this_thread::sleep_for(std::chrono::milliseconds(16));

            frame_count++;
            if (glfwGetTime() - time_begin >= 1.0)
            {
                std::string title(TITLE);
                glfwSetWindowTitle(window, (title + " (" + std::to_string(frame_count) + ')').c_str());
                time_begin = glfwGetTime();
                frame_count = 0;
            }
            //throw std::runtime_error("failed main loop");
        }
    }
    catch (const std::exception &e)
    {
        msg::error("Render error: ", e.what());
        std::cin.get();
    }
}

int main(int argc, char *argv[])
{
    msg::print("Program args: ");
    for(uint32_t i = 0; i < argc; i++){
        msg::print( *(argv + i));
        if(std::strcmp(*(argv + i),"debug") == 0) APP_DEBUG = true;
    } 
    msg::printl();
    
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // disable OpenGL
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);     // hide window while Vulkan initialises

    window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
    glfwSetWindowTitle(window, TITLE);

    // input
    glfwSetCursorPosCallback(window, Input::mouse_move_callback);
    glfwSetMouseButtonCallback(window, Input::mouse_press_callback);
    glfwSetScrollCallback(window, Input::mouse_scroll_callback);
    glfwSetKeyCallback(window, Input::key_press_callback);
    
    // move window to the middle of the screen
    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (mode->width - 800) / 2, (mode->height - 600) / 2);

    try
    {   
        uint64_t start = timestamp_milli();
        app.init_vulkan(window);
        msg::success("Program load: SUCCESS (", (float)(timestamp_milli() - start)/1000,")");
    }
    catch (const std::exception &e)
    {
        msg::error("Initialisation error: ", e.what());
        std::cin.get();
        return EXIT_FAILURE;
    }

    glfwShowWindow(window);

    std::thread render_loop(render_thread_function);

    while (APP_RUNNING)
    {
        glfwPollEvents();
        if(RECREATE_SWAPCHAIN){ app.recreate_swapchain(); RECREATE_SWAPCHAIN = false; }
        APP_RUNNING = !glfwWindowShouldClose(window);
    }

    render_loop.join();

    app.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
    msg::success("Program exit!");
    std::cin.get();
    return EXIT_SUCCESS;

}
/*
https://github.com/SaschaWillems/Vulkan/blob/master/examples/hdr/hdr.cpp
vk_sharing_mode_

https://streamable.com/us243l
shaderSampledImageArrayDynamicIndexing 
vkCmdPushConstants()
https://www.reddit.com/r/vulkan/comments/86tna1/question_about_descriptor_sets/
https://github.com/KhronosGroup/glTF-Sample-Models/tree/master/2.0
*/