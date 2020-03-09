#ifdef NDEBUG // C++ standart macro
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

#include "common.hpp"
#include "application.hpp"

void testing(){
    std::cout<< VK_API_VERSION_1_0 << std::endl;
    std::cout<< VK_MAKE_VERSION(1,0,0) << std::endl;
    std::cout<< VK_MAKE_VERSION( 1, 0, VK_HEADER_VERSION ) << std::endl;
    std::cin.get();
}

#ifdef NDEBUG 
    int WinMain(){
#else
    int main() {
#endif

    //testing();
    //return EXIT_SUCCESS;

    Application app;
    
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cin.get();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}