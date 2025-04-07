#pragma once
#include <kitsune_engine.hpp>

KitsuneEngine::KitsuneEngine(KitsuneWindowing& windowing): windowing_(windowing) {}
KitsuneEngine::~KitsuneEngine() {}

void KitsuneEngine::Init()
{
	windowing_.init();
}

void KitsuneEngine::CreateContext()
{
	auto vkGetInstanceProcAddr = windowing_.GetVkGetInstanceProcAddr();
	resorces.context.emplace(vkGetInstanceProcAddr);
}

void KitsuneEngine::CreateInstance()
{
    std::vector<vk::ExtensionProperties> availableExtensions = resorces.context->enumerateInstanceExtensionProperties();
    std::vector<const char*> requiredExtensions = GetRequiredInstanceExtensions(availableExtensions);

    if (!AreExtensionsSupported(requiredExtensions, availableExtensions)) {
        throw std::runtime_error("Required instance extensions are missing.");
    }

    fmt::println("Required instance extensions:");
    for (const auto& ext : requiredExtensions) {
        fmt::println("  {}", ext);
    }

    vk::ApplicationInfo appInfo{};
    appInfo.setPApplicationName(ENGINE_NAME)
        .setApplicationVersion(ENGINE_VERSION)
        .setPEngineName(ENGINE_NAME)
        .setEngineVersion(ENGINE_VERSION)
        .setApiVersion(API_VERSION);

    vk::InstanceCreateInfo createInfo{};
    createInfo.setPApplicationInfo(&appInfo)
        .setEnabledExtensionCount(static_cast<uint32_t>(requiredExtensions.size()))
        .setPpEnabledExtensionNames(requiredExtensions.data());

    if (hasPortability) {
        createInfo.setFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR);
    }


    resorces.instance.emplace(*resorces.context, createInfo);
}

// Utility Methods
std::vector<const char*> KitsuneEngine::GetRequiredInstanceExtensions(const std::vector<vk::ExtensionProperties>& available) {
    std::vector<const char*> extensions;

    windowing_.GetInstanceExtensions(extensions);




#ifdef VKB_ENABLE_PORTABILITY
    extensions.push_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName);
    hasPortability = std::any_of(available.begin(), available.end(),
        [](const auto& ext) { return strcmp(ext.extensionName, vk::KHRPortabilityEnumerationExtensionName) == 0; });
    if (hasPortability) {
        extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    }
#endif

    return extensions;
}

bool KitsuneEngine::AreExtensionsSupported(const std::vector<const char*>& required, const std::vector<vk::ExtensionProperties>& available) const {
	for (const auto* req : required) {
		bool found = false;
		for (const auto& avail : available) {
			if (strcmp(req, avail.extensionName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			fmt::println("Missing extension: {}", req);
			return false;
		}
	}
	return true;
}