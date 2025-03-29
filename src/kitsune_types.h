#pragma once
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
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
#include <utility>;
#include <cassert>
#include <fstream>
#include <vulkan/vulkan_raii.hpp>


#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_filesystem.h>

#include <fmt/core.h>

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
             fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)