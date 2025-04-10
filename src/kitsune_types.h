#pragma once
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
#define VKB_ENABLE_PORTABILITY


#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <cstring>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <set>
#include <algorithm>
#include <utility>
#include <cassert>
#include <fstream>
#include <vulkan/vulkan_raii.hpp>



#include <stdexcept>
#include <glm/glm.hpp>

#include <sstream>

#include <vulkan/vulkan_to_string.hpp>


#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_filesystem.h>

#include <fmt/core.h>

#define VK_CHECK(x)                                                     \
    do {                                                                \
        vk::Result err = x;                                               \
        if (err) {                                                      \
             fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)


static constexpr const char* ENGINE_NAME = "HelloTriangle";
static constexpr uint32_t ENGINE_VERSION = VK_MAKE_VERSION(1, 0, 0);
static constexpr uint32_t API_VERSION = VK_API_VERSION_1_3;
static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;