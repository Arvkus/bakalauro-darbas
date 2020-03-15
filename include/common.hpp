#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
// #include <vulkan/vulkan.h> // glfw includes vulkan

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <bitset>
#include <string>  
#include <chrono>

#include "utilities.hpp"