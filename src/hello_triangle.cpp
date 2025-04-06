#pragma once
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
#define VKB_ENABLE_PORTABILITY
#define VK_ENABLE_BETA_EXTENSIONS
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan_raii.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <fmt/core.h>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <stdexcept>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <vulkan/vulkan_to_string.hpp>

// Debug callback for validation layers (enabled in debug builds)
#if defined(_DEBUG)
VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageFunc(vk::DebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT              messageTypes,
    vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* /*pUserData*/)
{
    std::ostringstream message;

    message << vk::to_string(messageSeverity) << ": " << vk::to_string(messageTypes) << ":\n";
    message << std::string("\t") << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
    message << std::string("\t") << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
    message << std::string("\t") << "message         = <" << pCallbackData->pMessage << ">\n";
    if (0 < pCallbackData->queueLabelCount)
    {
        message << std::string("\t") << "Queue Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++)
        {
            message << std::string("\t\t") << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->cmdBufLabelCount)
    {
        message << std::string("\t") << "CommandBuffer Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++)
        {
            message << std::string("\t\t") << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->objectCount)
    {
        message << std::string("\t") << "Objects:\n";
        for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
        {
            message << std::string("\t\t") << "Object " << i << "\n";
            message << std::string("\t\t\t") << "objectType   = " << vk::to_string(pCallbackData->pObjects[i].objectType) << "\n";
            message << std::string("\t\t\t") << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
            if (pCallbackData->pObjects[i].pObjectName)
            {
                message << std::string("\t\t\t") << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
            }
        }
    }

#ifdef _WIN32
    MessageBox(NULL, message.str().c_str(), "Alert", MB_OK);
#else
    fmt::printIn("{}", message.str());
#endif

    return false;
}
#endif

class HelloTriangle {

    

    // Constants
    static constexpr const char* ENGINE_NAME = "HelloTriangle";
    static constexpr uint32_t ENGINE_VERSION = VK_MAKE_VERSION(1, 0, 0);
    static constexpr uint32_t API_VERSION = VK_API_VERSION_1_3;
    static constexpr std::array<const char*, 2> REQUIRED_DEVICE_EXTENSIONS = { vk::KHRSwapchainExtensionName, vk::KHRSynchronization2ExtensionName };
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    // SDL and Window
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{ nullptr, SDL_DestroyWindow };
    std::string basePath;

    // Vulkan Core Objects
    std::optional<vk::raii::Context> vkContext{};
    std::optional<vk::raii::Instance> vkInstance{};
    VkDebugUtilsMessengerEXT vkDebugMessenger{ VK_NULL_HANDLE }; // Raw handle, managed by instance
    std::optional<vk::raii::SurfaceKHR> vkSurface{};
    std::optional<vk::raii::PhysicalDevice> vkPhysicalDevice{};
    std::optional<vk::raii::Device> vkDevice{};

    // Queue Indices and Objects
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
    } queueFamilyIndices{ std::nullopt, std::nullopt };
    std::optional<vk::raii::Queue> graphicsQueue{};
    std::optional<vk::raii::Queue> presentQueue{};

    // Swapchain and Related Resources
    std::optional<vk::raii::SwapchainKHR> vkSwapchain{};
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::raii::ImageView> swapchainImageViews;
    vk::Format swapchainFormat{ vk::Format::eUndefined };
    vk::Extent2D swapchainExtent{ 0, 0 };

    // Rendering Resources
    std::optional<vk::raii::PipelineLayout> vkPipelineLayout{};
    std::optional<vk::raii::Pipeline> vkGraphicsPipeline{};
    std::vector<vk::raii::Framebuffer> framebuffers;

    // Command and Synchronization Objects
    std::optional<vk::raii::CommandPool> vkCommandPool{};
    std::vector<vk::raii::CommandBuffer> commandBuffers;
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    // Runtime State
    vk::Extent2D windowExtent{ 800, 600 };
    bool isRunning{ true };
    bool isFramebufferResized{ false };
    bool isRenderingEnabled{ false };
    uint32_t currentFrame{ 0 };
    uint32_t currentImage{ 0 };
    bool useVsync{ true };
    bool hasPortability{ false };
    bool hasDebugUtils{ false };

public:
    HelloTriangle() = default;
    ~HelloTriangle() { cleanup(); }

    void initialize() {
        initializeSDL();
        initializeVulkan();
    }

    void run() {
        SDL_ShowWindow(window.get());
        SDL_MaximizeWindow(window.get());
        isRenderingEnabled = true;

        while (isRunning) {
            processEvents();
            if (windowExtent.width > 0 && windowExtent.height > 0 && isRenderingEnabled) {
                renderFrame();
            }
        }
        if (vkDevice) vkDevice->waitIdle();
    }

private:
    class SDLException : public std::runtime_error {
    public:
        explicit SDLException(const std::string& msg) : std::runtime_error(msg + ": " + SDL_GetError()) {}
    };

    // Initialization Methods
    void initializeSDL() {
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

        windowExtent = getWindowExtent();
        SDL_SetWindowMinimumSize(window.get(), 100, 100);
        SDL_SetWindowPosition(window.get(), 2, 32);

        basePath = SDL_GetBasePath() ? SDL_GetBasePath() : "./";
        fmt::println("Base path: {}", basePath);
    }

    void initializeVulkan() {
        createVulkanContext();
        createInstance();
        createSurface();
        selectPhysicalDevice();
        createLogicalDevice();
        createCommandPool();
        createSwapchain();
        createImageViews();
        createGraphicsPipeline();
        createFramebuffers();
        createSynchronizationObjects();
        createCommandBuffers();
    }

    void createVulkanContext() {
        auto vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
        if (!vkGetInstanceProcAddr) throw SDLException("Failed to get vkGetInstanceProcAddr");
        vkContext.emplace(vkGetInstanceProcAddr);
    }

    void createInstance() {
        std::vector<vk::ExtensionProperties> availableExtensions = vkContext->enumerateInstanceExtensionProperties();
        std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions(availableExtensions);

        if (!areExtensionsSupported(requiredExtensions, availableExtensions)) {
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

        vk::DebugUtilsMessengerCreateInfoEXT messengerInfo{};
        
#ifdef _DEBUG
        if (hasDebugUtils) {
            
            messengerInfo.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
                .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
                .setPfnUserCallback(&debugMessageFunc);

            // Temporarily link to instance creation for early validation (optional)
            createInfo.setPNext(&messengerInfo);
        }
#endif // _DEBUG

        


        vkInstance.emplace(*vkContext, createInfo);

#ifdef _DEBUG
        vk::raii::DebugUtilsMessengerEXT tempMessenger(*vkInstance, messengerInfo);
        vkDebugMessenger = tempMessenger.release(); // Instance will destroy it
#endif // DEBUG

    }
    

    void createSurface() {
        VkSurfaceKHR rawSurface;
        if (!SDL_Vulkan_CreateSurface(window.get(), **vkInstance, nullptr, &rawSurface)) {
            throw SDLException("Failed to create surface");
        }
        vkSurface.emplace(*vkInstance, rawSurface);
    }

    void selectPhysicalDevice() {
        auto devices = vkInstance->enumeratePhysicalDevices();
        for (const auto& device : devices) {
            queueFamilyIndices = findQueueFamilies(device);
            if (queueFamilyIndices.graphics && queueFamilyIndices.present) {
                vkPhysicalDevice = device;
                break;
            }
        }
        if (!vkPhysicalDevice) throw std::runtime_error("No suitable physical device found");
    }

    void createLogicalDevice() {
        std::vector<vk::ExtensionProperties> availableExtensions = vkPhysicalDevice->enumerateDeviceExtensionProperties();
        std::vector<const char*> requiredExtensions{ REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end() };

        if (!areExtensionsSupported(requiredExtensions, availableExtensions)) {
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

        vkDevice.emplace(*vkPhysicalDevice, createInfo);
        graphicsQueue.emplace(*vkDevice, *queueFamilyIndices.graphics, 0);
        presentQueue.emplace(*vkDevice, *queueFamilyIndices.present, 0);
    }

    void createCommandPool() {
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.setQueueFamilyIndex(*queueFamilyIndices.graphics)
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        vkCommandPool.emplace(*vkDevice, poolInfo);
    }

    void createSwapchain() {
        vk::SurfaceCapabilitiesKHR capabilities = vkPhysicalDevice->getSurfaceCapabilitiesKHR(*vkSurface);
        auto formats = vkPhysicalDevice->getSurfaceFormatsKHR(*vkSurface);
        auto presentModes = vkPhysicalDevice->getSurfacePresentModesKHR(*vkSurface);

        vk::SurfaceFormatKHR surfaceFormat = chooseSwapchainFormat(formats);
        swapchainFormat = surfaceFormat.format; // Extract vk::Format
        vk::PresentModeKHR presentMode = choosePresentMode(presentModes);
        swapchainExtent = chooseSwapchainExtent(capabilities);

        uint32_t imageCount = capabilities.minImageCount == 1 ? 2 : capabilities.minImageCount;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo{};
        createInfo.setSurface(*vkSurface)
            .setMinImageCount(imageCount)
            .setImageFormat(swapchainFormat)
            .setImageColorSpace(surfaceFormat.colorSpace) // Use colorSpace from surfaceFormat
            .setImageExtent(swapchainExtent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setPreTransform(capabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(presentMode)
            .setClipped(true);

        std::array<uint32_t, 2> queueIndices = { *queueFamilyIndices.graphics, *queueFamilyIndices.present };
        if (queueFamilyIndices.graphics != queueFamilyIndices.present) {
            createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(2)
                .setPQueueFamilyIndices(queueIndices.data());
        }
        else {
            createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        }

        vkSwapchain.emplace(*vkDevice, createInfo);
        swapchainImages = vkSwapchain->getImages();
    }

    void createImageViews() {
        swapchainImageViews.clear();
        swapchainImageViews.reserve(swapchainImages.size());
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.setViewType(vk::ImageViewType::e2D)
            .setFormat(swapchainFormat)
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        for (const auto& image : swapchainImages) {
            viewInfo.setImage(image);
            swapchainImageViews.emplace_back(*vkDevice, viewInfo);
        }
    }



    void createGraphicsPipeline() {
        auto vertCode = loadShader("shaders/shader.vert.spv");
        auto fragCode = loadShader("shaders/shader.frag.spv");
        vk::raii::ShaderModule vertModule(*vkDevice, vk::ShaderModuleCreateInfo{ {}, vertCode.size(), reinterpret_cast<const uint32_t*>(vertCode.data()) });
        vk::raii::ShaderModule fragModule(*vkDevice, vk::ShaderModuleCreateInfo{ {}, fragCode.size(), reinterpret_cast<const uint32_t*>(fragCode.data()) });

        std::array<vk::PipelineShaderStageCreateInfo, 2> stages = {
            vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eVertex, *vertModule, "main" },
            vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eFragment, *fragModule, "main" }
        };

        vk::PipelineVertexInputStateCreateInfo vertexInput{};
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList);

        vk::PipelineViewportStateCreateInfo viewportState{};
        viewportState.setViewportCount(1).setScissorCount(1);

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.setPolygonMode(vk::PolygonMode::eFill).setLineWidth(1.0f);

        vk::PipelineMultisampleStateCreateInfo multisample{};
        multisample.setRasterizationSamples(vk::SampleCountFlagBits::e1);

        vk::PipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

        vk::PipelineColorBlendStateCreateInfo blendState{};
        blendState.setAttachmentCount(1).setPAttachments(&blendAttachment);

        std::array dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eCullMode,
                                     vk::DynamicState::eFrontFace, vk::DynamicState::ePrimitiveTopology };
        vk::PipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size())).setPDynamicStates(dynamicStates.data());

        vkPipelineLayout.emplace(*vkDevice, vk::PipelineLayoutCreateInfo{});

        vk::PipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.setColorAttachmentCount(1).setPColorAttachmentFormats(&swapchainFormat);

        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.setStageCount(static_cast<uint32_t>(stages.size()))
            .setPStages(stages.data())
            .setPVertexInputState(&vertexInput)
            .setPInputAssemblyState(&inputAssembly)
            .setPViewportState(&viewportState)
            .setPRasterizationState(&rasterizer)
            .setPMultisampleState(&multisample)
            .setPColorBlendState(&blendState)
            .setPDynamicState(&dynamicState)
            .setLayout(**vkPipelineLayout)
            .setPNext(&renderingInfo);

        vkGraphicsPipeline.emplace(*vkDevice, nullptr, pipelineInfo);
    }

    void createFramebuffers() {
        framebuffers.clear();
        framebuffers.reserve(swapchainImageViews.size());
        for (const auto& view : swapchainImageViews) {
            vk::FramebufferCreateInfo fbInfo{};
            fbInfo
                .setAttachmentCount(1)
                .setPAttachments(&(*view))
                .setWidth(swapchainExtent.width)
                .setHeight(swapchainExtent.height)
                .setLayers(1);
            framebuffers.emplace_back(*vkDevice, fbInfo);
        }
    }

    void createSynchronizationObjects() {
        vk::SemaphoreCreateInfo semaphoreInfo{};
        vk::FenceCreateInfo fenceInfo{ vk::FenceCreateFlagBits::eSignaled };

        imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            imageAvailableSemaphores.emplace_back(*vkDevice, semaphoreInfo);
            renderFinishedSemaphores.emplace_back(*vkDevice, semaphoreInfo);
            inFlightFences.emplace_back(*vkDevice, fenceInfo);
        }
    }

    void createCommandBuffers() {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.setCommandPool(*vkCommandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(MAX_FRAMES_IN_FLIGHT);
        commandBuffers = vkDevice->allocateCommandBuffers(allocInfo);
    }

    // Rendering Methods
    void renderFrame() {
        auto waitResult = vkDevice->waitForFences(*inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        auto [result, imageIndex] = vkSwapchain->acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[currentFrame], nullptr);
        currentImage = imageIndex;

        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapchain();
            return;
        }
        else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("Failed to acquire swapchain image");
        }

        vkDevice->resetFences(*inFlightFences[currentFrame]);
        vk::raii::CommandBuffer& cmd = commandBuffers[currentFrame];
        cmd.reset();

        vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        cmd.begin(beginInfo);

        transitionImageLayout(cmd, imageIndex,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
            vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        vk::RenderingInfo renderingInfo = getRenderingInfo(imageIndex);
        cmd.beginRendering(renderingInfo);

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *vkGraphicsPipeline);
        cmd.setViewport(0, vk::Viewport{ 0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0f, 1.0f });
        cmd.setScissor(0, vk::Rect2D{ {0, 0}, swapchainExtent });
        cmd.setCullMode(vk::CullModeFlagBits::eNone);
        cmd.setFrontFace(vk::FrontFace::eCounterClockwise);
        cmd.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
        cmd.draw(3, 1, 0, 0);
        cmd.endRendering();

        transitionImageLayout(cmd, imageIndex,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe);

        cmd.end();

        vk::SubmitInfo submitInfo{};
        vk::PipelineStageFlags waitStage{ vk::PipelineStageFlagBits::eTopOfPipe }; // Fixed from TopOfPipe
        submitInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&(*imageAvailableSemaphores[currentFrame]))
            .setPWaitDstStageMask(&waitStage)
            .setCommandBufferCount(1)
            .setPCommandBuffers(&(*cmd))
            .setSignalSemaphoreCount(1)
            .setPSignalSemaphores(&(*renderFinishedSemaphores[currentFrame]));

        graphicsQueue->submit(submitInfo, *inFlightFences[currentFrame]);

        vk::Result presentResult = presentImage(imageIndex);
        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
            recreateSwapchain();
        }
        else if (presentResult != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to present swapchain image");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }


    vk::RenderingInfo getRenderingInfo(uint32_t imageIndex) const {
        vk::RenderingAttachmentInfo colorAttachment{};
        colorAttachment.setImageView(*swapchainImageViews[imageIndex])
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue({ std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.0f} });

        vk::RenderingInfo renderingInfo{};
        renderingInfo.setRenderArea({ {0, 0}, swapchainExtent })
            .setLayerCount(1)
            .setColorAttachmentCount(1)
            .setPColorAttachments(&colorAttachment);
        return renderingInfo;
    }

    vk::Result presentImage(uint32_t imageIndex) {
        vk::PresentInfoKHR presentInfo{};
        presentInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&(*renderFinishedSemaphores[currentFrame]))
            .setSwapchainCount(1)
            .setPSwapchains(&(**vkSwapchain))
            .setPImageIndices(&imageIndex);
        return presentQueue->presentKHR(presentInfo);
    }

    void transitionImageLayout(const vk::raii::CommandBuffer& cmd, uint32_t imageIndex,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccess, vk::AccessFlags2 dstAccess,
        vk::PipelineStageFlags2 srcStage, vk::PipelineStageFlags2 dstStage) const {
        vk::ImageMemoryBarrier2 barrier{};
        barrier.setImage(swapchainImages[imageIndex])
            .setOldLayout(oldLayout)
            .setNewLayout(newLayout)
            .setSrcStageMask(srcStage)
            .setDstStageMask(dstStage)
            .setSrcAccessMask(srcAccess)
            .setDstAccessMask(dstAccess)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        vk::DependencyInfo dependency{};
        dependency.setImageMemoryBarrierCount(1)
            .setPImageMemoryBarriers(&barrier);
        cmd.pipelineBarrier2(dependency);
    }

    

    void recreateSwapchain() {
        vkDevice->waitIdle();
        windowExtent = getWindowExtent();
        if (windowExtent.width == 0 || windowExtent.height == 0) return;

        framebuffers.clear();
        swapchainImageViews.clear();
        vkSwapchain.reset();

        createSwapchain();
        createImageViews();
        createFramebuffers();
        isFramebufferResized = false;
    }

    // Event Handling
    void processEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                isRunning = false;
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                windowExtent = getWindowExtent();
                fmt::println("Window resized: {}x{}", windowExtent.width, windowExtent.height);
                isFramebufferResized = true;
                recreateSwapchain();
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                isRenderingEnabled = false;
                break;
            case SDL_EVENT_WINDOW_RESTORED:
                isRenderingEnabled = true;
                break;
            }
        }
    }

    // Utility Methods
    std::vector<const char*> getRequiredInstanceExtensions(const std::vector<vk::ExtensionProperties>& available) {
        std::vector<const char*> extensions;

        const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(nullptr);
        if (!sdlExts) throw SDLException("Failed to get SDL Vulkan extensions");
        extensions.push_back(*sdlExts);

#ifdef VK_USE_PLATFORM_WIN32_KHR
        extensions.push_back(vk::KHRWin32SurfaceExtensionName);
#endif

#if defined(_DEBUG)
        hasDebugUtils = std::any_of(available.begin(), available.end(),
            [](const auto& ext) { return strcmp(ext.extensionName, vk::EXTDebugUtilsExtensionName) == 0; });
        if (hasDebugUtils) {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }
        else {
            fmt::println("{} not available; debug utils disabled", vk::EXTDebugUtilsExtensionName);
        }
#endif

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

    bool areExtensionsSupported(const std::vector<const char*>& required, const std::vector<vk::ExtensionProperties>& available) {
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

    QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& device) {
        QueueFamilyIndices indices;
        auto families = device.getQueueFamilyProperties();
        for (uint32_t i = 0; i < families.size(); ++i) {
            if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) indices.graphics = i;
            if (device.getSurfaceSupportKHR(i, *vkSurface)) indices.present = i;
            if (indices.graphics && indices.present) break;
        }
        return indices;
    }

    vk::SurfaceFormatKHR chooseSwapchainFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const {
        for (const auto& format : formats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return format;
            }
        }
        return formats[0];
    }

    vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& modes) const {
        return useVsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate;
    }

    vk::Extent2D chooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR& caps) const {
        if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return caps.currentExtent;
        }
        vk::Extent2D extent = getWindowExtent();
        extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
        return extent;
    }

    vk::Extent2D getWindowExtent() const {
        int w, h;
        SDL_GetWindowSizeInPixels(window.get(), &w, &h);
        return { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
    }

    std::vector<char> loadShader(const std::string& filename) const {
        std::string path = basePath + filename;
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Failed to open shader: " + path);
        size_t size = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(size);
        file.seekg(0);
        file.read(buffer.data(), size);
        file.close();
        return buffer;
    }

    void cleanup() {
        SDL_Quit();
    }
};

int main() {
    try {
        HelloTriangle app;
        app.initialize();
        app.run();
    }
    catch (const std::exception& e) {
        fmt::println("Error: {}", e.what());
        return 1;
    }
    return 0;
}

