const bool isDebug = true;
#include "common.hpp"
//#include "application.hpp"
//Application app;




int main(){
    

    glfwInit();
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::cin.get();
}




/*
int main() {

    Application app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cin.get();
        return EXIT_FAILURE;
    }
    std::cin.get();
    return EXIT_SUCCESS;
    
    
}
*/