#pragma once
#include <kitsune_windowing.hpp>

    
KitsuneWindowing::KitsuneWindowing() {}
KitsuneWindowing::~KitsuneWindowing() {}


void KitsuneWindowing::init()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) throw SDLException("Failed to initialize SDL");
    if (!SDL_Vulkan_LoadLibrary(nullptr)) throw SDLException("Failed to load Vulkan library");

    SDL_DisplayID primary = SDL_GetPrimaryDisplay();
    SDL_Rect usableBounds{};
    SDL_GetDisplayUsableBounds(primary, &usableBounds);
    fmt::println("Display usable bounds: {}x{}", usableBounds.w, usableBounds.h);

    window.reset(SDL_CreateWindow(
        ENGINE_NAME,
        usableBounds.w - 4, usableBounds.h - 34, // Account for title bar and resize handle
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN));
    if (!window) throw SDLException("Failed to create window");

    SDL_SetWindowMinimumSize(window.get(), 100, 100);
    SDL_SetWindowPosition(window.get(), 2, 32);
}

void KitsuneWindowing::GetInstanceExtensions(std::vector<const char*>& extensions) const
{
    const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(nullptr);
    if (!sdlExts) throw SDLException("Failed to get SDL Vulkan extensions");

    for (const char* const* ext = sdlExts; *ext != nullptr; ++ext) {
        extensions.push_back(*ext);
        fmt::println("SDL extension: {}", *ext);
    }
}

vk::Extent2D KitsuneWindowing::GetWindowExtent() const {
    int w, h;
    SDL_GetWindowSizeInPixels(window.get(), &w, &h);
    return { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
}

VkSurfaceKHR KitsuneWindowing::GetSurface(const vk::raii::Instance& vkInstance) const
{
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(window.get(), *vkInstance, nullptr, &surface))
    {
        throw SDLException("Failed to create surface");
    }
    return surface;
}

PFN_vkGetInstanceProcAddr KitsuneWindowing::GetVkGetInstanceProcAddr() const
{
    auto instanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());

    if (!instanceProcAddr) throw SDLException("Failed to get vkGetInstanceProcAddr");

    return instanceProcAddr;
}