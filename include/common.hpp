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

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

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

const bool APP_DEBUG = true;
const char* TITLE = "Vulkan window";
const int MAX_FRAMES_IN_FLIGHT = 2;
bool APP_RUNNING = true;
bool RECREATE_SWAPCHAIN = false;
HANDLE H_CONSOLE = GetStdHandle(STD_OUTPUT_HANDLE);

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
    "VK_KHR_swapchain",
};

//-----------------------------------------
// Utility functions/structs/enums

#include "utilities.hpp"