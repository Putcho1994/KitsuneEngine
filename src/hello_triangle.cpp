#pragma once
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
#define VKB_ENABLE_PORTABILITY
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif 


#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan.hpp>
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



class HelloTriangle {
    static inline constexpr const char* ENGINE_NAME = "HelloTriangle";
    static inline constexpr uint32_t ENGINE_VERSION = VK_MAKE_VERSION(1, 0, 0);
    static inline constexpr uint32_t API_VERSION = VK_API_VERSION_1_4;
    static inline constexpr std::array<const char* const, 2> REQUIRED_EXTENSIONS = { vk::KHRSwapchainExtensionName, vk::KHRSynchronization2ExtensionName };
    static inline constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{ nullptr, SDL_DestroyWindow };
    std::optional<vk::raii::Context> context{};
    std::optional<vk::raii::Instance> instance{};
    std::optional<vk::raii::SurfaceKHR> surface{};
    std::optional<vk::raii::PhysicalDevice> physicalDevice{};
    std::pair<std::optional<uint32_t>, std::optional<uint32_t>> deviceIndices{ std::nullopt, std::nullopt };
    std::optional<vk::raii::Device> device{};
    std::optional<vk::raii::Queue> graphicsQueue{};
    std::optional<vk::raii::Queue> presentQueue{};
    std::optional<vk::raii::SwapchainKHR> swapChain{};
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::raii::ImageView> swapchainImageViews;
    vk::Format swapChainImageFormat{ vk::Format::eUndefined };
    vk::Extent2D swapChainExtent{ 0, 0 };
    std::optional<vk::raii::RenderPass> renderPass{};
    std::optional<vk::raii::PipelineLayout> pipelineLayout{};
    std::optional<vk::raii::Pipeline> graphicsPipeline{};
    std::optional<vk::raii::CommandPool> commandPool{};
    std::vector<vk::raii::Framebuffer> framebuffers;
    std::vector<vk::raii::CommandBuffer> commandBuffers;
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    std::string applicationPath;
    vk::Extent2D windowExtent{ 800, 600 };
    bool running{ true };
    bool framebufferResized{ false };
    uint32_t currentFrameIndex{ 0 };
    uint32_t currentImageIndex{ 0 };
    bool vsync{ true };

public:
    HelloTriangle() = default;
    ~HelloTriangle() { cleanup(); }

    void init() {
        if (!SDL_Init(SDL_INIT_VIDEO)) throw SDLException("Failed to initialize SDL");
        if (!SDL_Vulkan_LoadLibrary(nullptr)) throw SDLException("Failed to load Vulkan library");

        //init the minimum size to monitor size -4 on width and 80 (Title bar size -4) on height so that we have pixel space for resize handle
        //this is setup for win32, may be different on other OS

        //int displayCount;
        //SDL_DisplayID* displays = SDL_GetDisplays(&displayCount);
        //if (!displays || displayCount <= 0) return vk::Extent2D{ 800, 600 };
        //SDL_DisplayID primary = displays[0];
        //SDL_free(displays);

        SDL_DisplayID primary = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(primary);

        SDL_Rect display_rect{};
        SDL_GetDisplayBounds(primary, &display_rect);
        fmt::println("SDL_GetDisplayBounds: {},{}", display_rect.w, display_rect.h);
        SDL_GetDisplayUsableBounds(primary, &display_rect);
        fmt::println("SDL_GetDisplayUsableBounds: {},{}", display_rect.w, display_rect.h);

        window.reset(SDL_CreateWindow("Hello Triangle",
            display_rect.w - 4, display_rect.h - 30 - 4,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN));
        if (!window) throw SDLException("Failed to create window");

        windowExtent = getWindowExtents();
        

        SDL_SetWindowMinimumSize(window.get(), 100, 100);

        SDL_SetWindowPosition(window.get(), 2, 32);

        applicationPath = SDL_GetBasePath() ? SDL_GetBasePath() : "./";
        fmt::println("Application path: {}", applicationPath);

        auto vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
        if (!vkGetInstanceProcAddr) throw SDLException("Failed to get vkGetInstanceProcAddr");
        context.emplace(vkGetInstanceProcAddr);

        initVulkan();
    }

    void run() 
    {
        SDL_ShowWindow(window.get());
        
        SDL_MaximizeWindow(window.get());

        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                handleEvent(&event);
            }
            if (windowExtent.width > 0 && windowExtent.height > 0) {
                drawFrame();
            }
        }
        if (device) device->waitIdle();
    }

private:
    class SDLException : public std::runtime_error {
    public:
        explicit SDLException(const std::string& msg) : std::runtime_error(msg + ": " + SDL_GetError()) {}
    };

    void initVulkan() {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createDevice();
        createCommandPool();
        createSwapchain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createSyncObjects();
        createCommandBuffers();
    }

    bool validateExtensions(const std::vector<const char*>& required, const std::vector<vk::ExtensionProperties>& available)
    {
        bool allFound = true;

        for (const auto* extensionName : required)
        {
            bool found = false;

            for (const auto& availableExtension : available)
            {
                if (strcmp(availableExtension.extensionName, extensionName) == 0)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                fmt::println("Error: Required extension not found: {}", extensionName);

                allFound = false;
            }

        }

        return allFound;
    }

    std::vector<const char*> getInstanceExtensions( std::vector<vk::ExtensionProperties>const& availableInstanceExtensions,bool & portabilityEnumerationAvailable) const
    {
        std::vector<const char*> required;

        const char* const* exts = SDL_Vulkan_GetInstanceExtensions(nullptr);
        if (!exts)
        {
            throw SDLException("Failed to get Vulkan instance extensions");
        }
        else
        {
            required.push_back(*exts);
        }

#if defined( VK_USE_PLATFORM_ANDROID_KHR )
        required.push_back(vk::KHRAndroidSurfaceExtensionName);
#elif defined( VK_USE_PLATFORM_METAL_EXT )
        required.push_back(vk::EXTMetalSurfaceExtensionName);
#elif defined( VK_USE_PLATFORM_VI_NN )
        required.push_back(vk::NNViSurfaceExtensionName);
#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
        required_instance_extensions.push_back(vk::KHRWaylandSurfaceExtensionName);
#elif defined( VK_USE_PLATFORM_WIN32_KHR )
        required.push_back(vk::KHRWin32SurfaceExtensionName);
#elif defined( VK_USE_PLATFORM_XCB_KHR )
        required.push_back(vk::KHRXcbSurfaceExtensionName);
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
        required.push_back(vk::KHRXlibSurfaceExtensionName);
#elif defined( VK_USE_PLATFORM_XLIB_XRANDR_EXT )
        required.push_back(vk::EXTAcquireXlibDisplayExtensionName);
#else
#	pragma error Platform not supported
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
        required.push_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName);
        portabilityEnumerationAvailable = false;
        if (std::any_of(availableInstanceExtensions.begin(),
            availableInstanceExtensions.end(),
            [](VkExtensionProperties const& extension) { return strcmp(extension.extensionName, vk::KHRPortabilityEnumerationExtensionName) == 0; }))
        {
            required.push_back(vk::KHRPortabilityEnumerationExtensionName);
            portabilityEnumerationAvailable = true;
        }
#else
        portabilityEnumerationAvailable = false;
#endif

        return required;
    }

    void createInstance() 
    {
        bool portabilityEnumerationAvailable = false;
        std::vector<vk::ExtensionProperties> availableInstanceExtensions = context->enumerateInstanceExtensionProperties();
        std::vector<const char*> requiredInstanceExtensions = getInstanceExtensions(availableInstanceExtensions, portabilityEnumerationAvailable);
        
        if (!validateExtensions(requiredInstanceExtensions, availableInstanceExtensions))
        {
            throw std::runtime_error("Required instance extensions are missing.");
        }

        std::cout << "Required Instance Extensions:" << std::endl;
        for (auto const& ext : requiredInstanceExtensions)
        {
            fmt::println("{}", ext);
        }

        vk::ApplicationInfo appInfo{};
        appInfo.setPApplicationName(ENGINE_NAME)
            .setApplicationVersion(ENGINE_VERSION)
            .setPEngineName(ENGINE_NAME)
            .setEngineVersion(ENGINE_VERSION)
            .setApiVersion(API_VERSION);

        vk::InstanceCreateInfo createInfo{};
        createInfo.setPApplicationInfo(&appInfo)
            .setEnabledExtensionCount(static_cast<uint32_t>(requiredInstanceExtensions.size()))
            .setPpEnabledExtensionNames(requiredInstanceExtensions.data());

        if (portabilityEnumerationAvailable)
        {
            createInfo.setFlags({ vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR });
        }

        instance.emplace(*context, createInfo);
    }

    void createSurface() {
        VkSurfaceKHR rawSurface;
        if (!SDL_Vulkan_CreateSurface(window.get(), **instance, nullptr, &rawSurface))
            throw SDLException("Failed to create surface");
        surface.emplace(*instance, rawSurface);
    }

    void pickPhysicalDevice() {
        auto devices = instance->enumeratePhysicalDevices();
        for (const auto& dev : devices) {
            auto [graphicsIdx, presentIdx] = isDeviceSuitable(dev);
            if (graphicsIdx && presentIdx) {
                physicalDevice = dev;
                deviceIndices = { graphicsIdx, presentIdx };
                break;
            }
        }
        if (!physicalDevice) throw std::runtime_error("No suitable physical device found");
    }

    void createDevice() {
        std::set<uint32_t> uniqueQueueFamilies = { *deviceIndices.first, *deviceIndices.second };
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;
        for (uint32_t family : uniqueQueueFamilies) {
            vk::DeviceQueueCreateInfo queueInfo{};
            queueInfo.setQueueFamilyIndex(family)
                .setQueueCount(1)
                .setPQueuePriorities(&queuePriority);
            queueCreateInfos.push_back(queueInfo);
        }

        vk::PhysicalDeviceFeatures features{};
        vk::DeviceCreateInfo createInfo{};
        createInfo.setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
            .setPQueueCreateInfos(queueCreateInfos.data())
            .setEnabledExtensionCount(static_cast<uint32_t>(REQUIRED_EXTENSIONS.size()))
            .setPpEnabledExtensionNames(REQUIRED_EXTENSIONS.data())
            .setPEnabledFeatures(&features);

        device.emplace(*physicalDevice, createInfo);
        graphicsQueue.emplace(*device, *deviceIndices.first, 0);
        presentQueue.emplace(*device, *deviceIndices.second, 0);
    }

    void createCommandPool() {
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.setQueueFamilyIndex(*deviceIndices.first)
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        commandPool.emplace(*device, poolInfo);
    }

    void createSwapchain() {
        vk::SurfaceCapabilitiesKHR caps = physicalDevice->getSurfaceCapabilitiesKHR(*surface);
        auto formats = physicalDevice->getSurfaceFormatsKHR(*surface);
        auto presentModes = physicalDevice->getSurfacePresentModesKHR(*surface);

        vk::SurfaceFormatKHR format = chooseSwapFormat(formats);
        vk::PresentModeKHR mode = choosePresentMode(presentModes);
        swapChainExtent = chooseSwapExtent(caps);

        uint32_t imageCount = caps.minImageCount == 1 ? 2 : caps.minImageCount;
        if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;

        vk::SwapchainCreateInfoKHR createInfo{};
        createInfo.setSurface(*surface)
            .setMinImageCount(imageCount)
            .setImageFormat(format.format)
            .setImageColorSpace(format.colorSpace)
            .setImageExtent(swapChainExtent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setPreTransform(caps.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(mode)
            .setClipped(true);

        std::array<uint32_t, 2> queueFamilyIndices = { *deviceIndices.first, *deviceIndices.second };
        if (*deviceIndices.first != *deviceIndices.second) {
            createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(2)
                .setPQueueFamilyIndices(queueFamilyIndices.data());
        }
        else {
            createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        }

        swapChain.emplace(*device, createInfo);
        swapChainImages = swapChain->getImages();
        swapChainImageFormat = format.format;
    }

    void createImageViews() {
        swapchainImageViews.clear();
        swapchainImageViews.reserve(swapChainImages.size());
        vk::ImageViewCreateInfo createInfo{};
        createInfo.setViewType(vk::ImageViewType::e2D)
            .setFormat(swapChainImageFormat)
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        for (const auto& image : swapChainImages) {
            createInfo.setImage(image);
            swapchainImageViews.emplace_back(*device, createInfo);
        }
    }

    void createRenderPass() {
        vk::AttachmentDescription colorAttachment{};
        colorAttachment.setFormat(swapChainImageFormat)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };

        vk::SubpassDescription subpass{};
        subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachmentCount(1)
            .setPColorAttachments(&colorAttachmentRef);

        vk::SubpassDependency dependency{};
        dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcAccessMask(vk::AccessFlagBits::eNone)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

        vk::RenderPassCreateInfo renderPassInfo{};
        renderPassInfo.setAttachmentCount(1)
            .setPAttachments(&colorAttachment)
            .setSubpassCount(1)
            .setPSubpasses(&subpass)
            .setDependencyCount(1)
            .setPDependencies(&dependency);

        renderPass.emplace(*device, renderPassInfo);
    }

    void createGraphicsPipeline() {
        auto vertCode = readFile("shaders/shader.vert.spv");
        auto fragCode = readFile("shaders/shader.frag.spv");
        vk::raii::ShaderModule vertShaderModule{ *device, vk::ShaderModuleCreateInfo{{}, vertCode.size(), reinterpret_cast<const uint32_t*>(vertCode.data())} };
        vk::raii::ShaderModule fragShaderModule{ *device, vk::ShaderModuleCreateInfo{{}, fragCode.size(), reinterpret_cast<const uint32_t*>(fragCode.data())} };

        vk::PipelineShaderStageCreateInfo vertStage{};
        vertStage.setStage(vk::ShaderStageFlagBits::eVertex)
            .setModule(*vertShaderModule)
            .setPName("main");

        vk::PipelineShaderStageCreateInfo fragStage{};
        fragStage.setStage(vk::ShaderStageFlagBits::eFragment)
            .setModule(*fragShaderModule)
            .setPName("main");

        std::array stages = { vertStage, fragStage };

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList);

        vk::PipelineViewportStateCreateInfo viewportState{};
        viewportState.setViewportCount(1)
            .setScissorCount(1)
            .setPViewports(nullptr)
            .setPScissors(nullptr);

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.setPolygonMode(vk::PolygonMode::eFill)
            .setLineWidth(1.0f)
            .setCullMode(vk::CullModeFlagBits::eBack)
            .setFrontFace(vk::FrontFace::eClockwise);

        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.setRasterizationSamples(vk::SampleCountFlagBits::e1);

        vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

        vk::PipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.setAttachmentCount(1)
            .setPAttachments(&colorBlendAttachment);

        std::array dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        vk::PipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.setDynamicStateCount(dynamicStates.size())
            .setPDynamicStates(dynamicStates.data());

        pipelineLayout.emplace(*device, vk::PipelineLayoutCreateInfo{});

        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.setStageCount(stages.size())
            .setPStages(stages.data())
            .setPVertexInputState(&vertexInputInfo)
            .setPInputAssemblyState(&inputAssembly)
            .setPViewportState(&viewportState)
            .setPRasterizationState(&rasterizer)
            .setPMultisampleState(&multisampling)
            .setPColorBlendState(&colorBlending)
            .setPDynamicState(&dynamicState)
            .setLayout(**pipelineLayout)
            .setRenderPass(*renderPass)
            .setSubpass(0);

        graphicsPipeline.emplace(*device, nullptr, pipelineInfo);
    }

    void createFramebuffers() {
        framebuffers.clear();
        framebuffers.reserve(swapchainImageViews.size());
        for (const auto& view : swapchainImageViews) {
            vk::FramebufferCreateInfo fbInfo{};
            fbInfo.setRenderPass(*renderPass)
                .setAttachmentCount(1)
                .setPAttachments(&(*view))
                .setWidth(swapChainExtent.width)
                .setHeight(swapChainExtent.height)
                .setLayers(1);
            framebuffers.emplace_back(*device, fbInfo);
        }
    }

    void createSyncObjects() {
        vk::SemaphoreCreateInfo semaphoreInfo{};
        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            imageAvailableSemaphores.emplace_back(*device, semaphoreInfo);
            renderFinishedSemaphores.emplace_back(*device, semaphoreInfo);
            inFlightFences.emplace_back(*device, fenceInfo);
        }
    }

    void createCommandBuffers() {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.setCommandPool(*commandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(MAX_FRAMES_IN_FLIGHT);
        commandBuffers = device->allocateCommandBuffers(allocInfo);
    }

    void drawFrame() {
        vk::Result waitFenceResult = device->waitForFences(*inFlightFences[currentFrameIndex], VK_TRUE, UINT64_MAX);

        auto [result, imageIndex] = swapChain->acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[currentFrameIndex], nullptr);
        currentImageIndex = imageIndex;

        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapchain();
            return;
        }
        else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("Failed to acquire swapchain image");
        }

        device->resetFences(*inFlightFences[currentFrameIndex]);

        vk::raii::CommandBuffer& commandBuffer = commandBuffers[currentFrameIndex];
        commandBuffer.reset();

        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        commandBuffer.begin(beginInfo);

        vk::RenderPassBeginInfo rpInfo{};
        rpInfo.setRenderPass(*renderPass)
            .setFramebuffer(*framebuffers[imageIndex])
            .setRenderArea({ {0, 0}, swapChainExtent });
        vk::ClearValue clearValue{};
        clearValue.color = vk::ClearColorValue{ std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.0f} };
        rpInfo.setClearValueCount(1)
            .setPClearValues(&clearValue);

        commandBuffer.beginRenderPass(rpInfo, vk::SubpassContents::eInline);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
        commandBuffer.setViewport(0, vk::Viewport{ 0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f });
        commandBuffer.setScissor(0, vk::Rect2D{ {0, 0}, swapChainExtent });
        commandBuffer.draw(3, 1, 0, 0);
        commandBuffer.endRenderPass();
        commandBuffer.end();

        vk::SubmitInfo submitInfo{};
        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        submitInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&(*imageAvailableSemaphores[currentFrameIndex]))
            .setPWaitDstStageMask(&waitStage)
            .setCommandBufferCount(1)
            .setPCommandBuffers(&(*commandBuffer))
            .setSignalSemaphoreCount(1)
            .setPSignalSemaphores(&(*renderFinishedSemaphores[currentFrameIndex]));

        graphicsQueue->submit(submitInfo, *inFlightFences[currentFrameIndex]);

        vk::Result presentResult = presentImage(imageIndex);
        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
            recreateSwapchain();
        }
        else if (presentResult != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to present swapchain image");
        }

        currentFrameIndex = (currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    vk::Result presentImage(uint32_t imageIndex)
    {
        vk::PresentInfoKHR presentInfo{};
        presentInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&(*renderFinishedSemaphores[currentFrameIndex]))
            .setSwapchainCount(1)
            .setPSwapchains(&(**swapChain))
            .setPImageIndices(&imageIndex);

        return presentQueue->presentKHR(presentInfo);
    }

    void recreateSwapchain() {
        device->waitIdle();

        windowExtent = getWindowExtents();
        if (windowExtent.width == 0 || windowExtent.height == 0) return;

        framebuffers.clear();
        swapchainImageViews.clear();
        swapChain.reset();

        createSwapchain();
        createImageViews();
        createFramebuffers();
        framebufferResized = false;
    }

    void handleEvent(const SDL_Event* event) {
        switch (event->type) {
        case SDL_EVENT_QUIT:
            running = false;
            break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
            windowExtent = getWindowExtents();
            fmt::println("Initial window extent: {}x{}", windowExtent.width, windowExtent.height);
            framebufferResized = true;
            recreateSwapchain();
            break;
        }
    }

    void cleanup() {
        SDL_Quit();
    }

    vk::Extent2D getWindowExtents() const {
        int w, h;
        SDL_GetWindowSizeInPixels(window.get(), &w, &h);
        return vk::Extent2D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
    }


    vk::SurfaceFormatKHR chooseSwapFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const {
        for (const auto& format : formats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return format;
            }
        }
        return formats[0];
    }

    vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& modes) const {
        return vsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& caps) const {
        if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return caps.currentExtent;
        }
        vk::Extent2D extent = getWindowExtents();
        extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
        return extent;
    }

    std::pair<std::optional<uint32_t>, std::optional<uint32_t>> isDeviceSuitable(const vk::raii::PhysicalDevice& dev) {
        auto queueFamilies = dev.getQueueFamilyProperties();
        std::optional<uint32_t> graphicsIdx, presentIdx;
        for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
            if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) graphicsIdx = i;
            if (dev.getSurfaceSupportKHR(i, *surface)) presentIdx = i;
            if (graphicsIdx && presentIdx) break;
        }
        if (!graphicsIdx || !presentIdx) return { std::nullopt, std::nullopt };

        auto exts = dev.enumerateDeviceExtensionProperties();
        for (const char* reqExt : REQUIRED_EXTENSIONS) {
            bool found = false;
            for (const auto& ext : exts) {
                if (strcmp(reqExt, ext.extensionName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) return { std::nullopt, std::nullopt };
        }

        auto formats = dev.getSurfaceFormatsKHR(*surface);
        auto presentModes = dev.getSurfacePresentModesKHR(*surface);
        if (formats.empty() || presentModes.empty()) return { std::nullopt, std::nullopt };

        return { graphicsIdx, presentIdx };
    }

    std::vector<char> readFile(const std::string& filename) const {
        std::string fullPath = applicationPath + filename;
        std::ifstream file(fullPath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Failed to open file: " + fullPath);
        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }
};

int main() {
    try {
        HelloTriangle app;
        app.init();
        app.run();
    }
    catch (const std::exception& e) {
        fmt::println("Error: {}", e.what());
        return 1;
    }
    return 0;
}