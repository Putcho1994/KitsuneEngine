#pragma once
#include <kitsune_types.h>

class KitsuneWindowing
{
public:
	KitsuneWindowing();
	~KitsuneWindowing();

	void init();
	void ShowWindow() { SDL_ShowWindow(window.get()); }
	void MaximizeWindow() { SDL_MaximizeWindow(window.get()); }
	void MinimizeWindow() { SDL_MinimizeWindow(window.get()); }

	PFN_vkGetInstanceProcAddr GetVkGetInstanceProcAddr() const;

	void GetInstanceExtensions(std::vector<const char*>& extensions) const;

	VkSurfaceKHR GetSurface(const vk::raii::Instance& vkInstance) const;

	vk::Extent2D GetWindowExtent() const;

private:
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{ nullptr, SDL_DestroyWindow };

};

class SDLException : public std::runtime_error {
public:
	explicit SDLException(const std::string& msg) : std::runtime_error(msg + ": " + SDL_GetError()) {}
};