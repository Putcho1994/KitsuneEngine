#pragma once
#include <kitsune_engine.hpp>

KitsuneEngine::KitsuneEngine(KitsuneWindowing& windowing) : windowing_(windowing) {}
KitsuneEngine::~KitsuneEngine() {}

void KitsuneEngine::Init()
{
    windowExtent = windowing_.GetWindowExtent();

    basePath = SDL_GetBasePath() ? SDL_GetBasePath() : "./";
    fmt::println("Base path: {}", basePath);

    CreateContext();
    CreateInstance();
    CreateSurface();
    SelectPhysicalDevice();
    CreateLogicalDevice();
}

void KitsuneEngine::ResetWindowExtent()
{
    windowExtent = windowing_.GetWindowExtent();
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

void KitsuneEngine::CreateSurface()
{
    const vk::raii::Instance& vkInstance = *resorces.instance;
    VkSurfaceKHR rawSurface = windowing_.GetSurface(vkInstance);
    resorces.surface.emplace(vkInstance, rawSurface);
}

void KitsuneEngine::SelectPhysicalDevice()
{
    auto devices = resorces.instance->enumeratePhysicalDevices();
    for (const auto& device : devices) {
        queueFamilyIndices = FindQueueFamilies(device);
        if (queueFamilyIndices.graphics && queueFamilyIndices.present) {
            resorces.physicalDevice = device;
            break;
        }
    }
    if (!resorces.physicalDevice) throw std::runtime_error("No suitable physical device found");
}

void KitsuneEngine::CreateLogicalDevice() 
{
    std::vector<vk::ExtensionProperties> availableExtensions = resorces.physicalDevice->enumerateDeviceExtensionProperties();
    std::vector<const char*> requiredExtensions{ vk::KHRSwapchainExtensionName, vk::KHRSynchronization2ExtensionName };

    if (!AreExtensionsSupported(requiredExtensions, availableExtensions)) {
        throw std::runtime_error("Required device extensions are missing");
    }

    if (hasPortability) {
        if (std::any_of(availableExtensions.begin(), availableExtensions.end(),
            [](const auto& ext) { return strcmp(ext.extensionName, vk::KHRPortabilitySubsetExtensionName) == 0; })) {
            requiredExtensions.push_back(vk::KHRPortabilitySubsetExtensionName);
        }
    }

    std::set<uint32_t> uniqueFamilies = { *queueFamilyIndices.graphics, *queueFamilyIndices.present };
    std::vector<vk::DeviceQueueCreateInfo> queueInfos;
    float priority = 1.0f;
    for (uint32_t family : uniqueFamilies) {
        vk::DeviceQueueCreateInfo queueInfo{};
        queueInfo.setQueueFamilyIndex(family)
            .setQueueCount(1)
            .setPQueuePriorities(&priority);
        queueInfos.push_back(queueInfo);
    }

    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeatures{};
    dynamicStateFeatures.setExtendedDynamicState(true);

    vk::PhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.setPNext(&dynamicStateFeatures)
        .setSynchronization2(true)
        .setDynamicRendering(true);

    vk::PhysicalDeviceFeatures2 features{};
    features.setPNext(&vulkan13Features);

    vk::DeviceCreateInfo createInfo{};
    createInfo.setQueueCreateInfoCount(static_cast<uint32_t>(queueInfos.size()))
        .setPQueueCreateInfos(queueInfos.data())
        .setEnabledExtensionCount(static_cast<uint32_t>(requiredExtensions.size()))
        .setPpEnabledExtensionNames(requiredExtensions.data())
        .setPNext(&features);

    resorces.device.emplace(*resorces.physicalDevice, createInfo);
    resorces.graphicsQueue.emplace(*resorces.device, *queueFamilyIndices.graphics, 0);
    resorces.presentQueue.emplace(*resorces.device, *queueFamilyIndices.present, 0);
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

QueueFamilyIndices KitsuneEngine::FindQueueFamilies(const vk::raii::PhysicalDevice& device) const {
    QueueFamilyIndices indices;
    auto families = device.getQueueFamilyProperties();
    for (uint32_t i = 0; i < families.size(); ++i) {
        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) indices.graphics = i;
        if (device.getSurfaceSupportKHR(i, *resorces.surface)) indices.present = i;
        if (indices.graphics && indices.present) break;
    }
    return indices;
}