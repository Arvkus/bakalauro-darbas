#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
// #include <vulkan/vulkan.h> // glfw includes vulkan

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/scalar_multiplication.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_resize.h>

#include "pfd/portable-file-dialogs.h"
#pragma comment(lib,"user32.lib") 
#pragma comment(lib,"shell32.lib") 

#include <json/json.hpp>

#include <iostream>
#include <stdexcept>
#include <typeinfo>
#include <functional>
#include <cstdlib>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <algorithm>
#include <fstream>
#include <bitset>
#include <string>  
#include <chrono>
#include <array>
#include <set>
#include <thread>

//-----------------------------------------
// Globals

const uint32_t MAX_OBJECTS = 256;
const uint32_t MAX_IMAGES = 64;
const uint32_t MAX_IMAGE_SIZE = 256;
const uint32_t DYNAMIC_DESCRIPTOR_SIZE = 256;

bool APP_DEBUG = false;
const char* TITLE = "Vulkan window";
const int MAX_FRAMES_IN_FLIGHT = 2;
bool APP_RUNNING = true;
bool RECREATE_SWAPCHAIN = false;
HANDLE H_CONSOLE = GetStdHandle(STD_OUTPUT_HANDLE);
std::string FILE_PATH = "C:\\";

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
    "VK_KHR_swapchain",
    //"VK_EXT_validation_features"
};

//-----------------------------------------
// Utility functions/structs/enums

#include "utilities.hpp"