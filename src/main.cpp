#include "common.hpp"
#include "application.hpp"

Application app;
GLFWwindow *window;
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

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
        SetConsoleTextAttribute(hConsole, ConsoleColor::Red);
        std::cerr << "Render error: " << e.what() << std::endl;
        //std::cin.get();
        //glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // disable OpenGL
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);     // hide window while Vulkan initialises

    window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
    glfwSetWindowTitle(window, TITLE);

    // move window to the middle of the screen
    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (mode->width - 800) / 2, (mode->height - 600) / 2);

    try
    {
        app.init_vulkan(window);
    }
    catch (const std::exception &e)
    {
        SetConsoleTextAttribute(hConsole, ConsoleColor::Red);
        std::cerr << "Initialisation error: " << e.what() << std::endl;
        std::cin.get();
        return EXIT_FAILURE;
    }

    glfwShowWindow(window);

    std::thread render_loop(render_thread_function);

    while (APP_RUNNING)
    {
        glfwPollEvents();
        APP_RUNNING = !glfwWindowShouldClose(window);
    }

    render_loop.join();

    app.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cin.get();
    return EXIT_SUCCESS;

}

/* Render loop:
    1. process queue commands
    2. execute input commands
    3. render view
*/

/*
http://talpykla.elaba.lt/elaba-fedora/objects/elaba:15129878/datastreams/MAIN/content
http://talpykla.elaba.lt/elaba-fedora/objects/elaba:2170722/datastreams/MAIN/content
http://talpykla.elaba.lt/elaba-fedora/objects/elaba:1896427/datastreams/MAIN/content
http://talpykla.elaba.lt/elaba-fedora/objects/elaba:1797456/datastreams/MAIN/content
http://talpykla.elaba.lt/elaba-fedora/objects/elaba:1840488/datastreams/MAIN/content

vk_sharing_mode_
*/