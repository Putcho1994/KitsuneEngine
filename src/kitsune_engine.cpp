#include <kitsune_engine.hpp>
#include <kitsune_types.h>

void KitsuneEngine::init() {
    if (!SDL_Init(SDL_INIT_VIDEO))
        throw SDLException("Failed to initialize SDL");
    if (!SDL_Vulkan_LoadLibrary(nullptr))
        throw SDLException("Failed to load Vulkan library");

    vk::Extent2D monitorExtent = get_monitor_extents();
    window.reset(SDL_CreateWindow("Kitsune Engine",
        monitorExtent.width-2, monitorExtent.height-80,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_HIDDEN));
    if (!window)
        throw SDLException("Failed to create window");

    _windowExtent = get_window_extents();
    fmt::println("Initial window extent (framebuffer): {}x{}", _windowExtent.width, _windowExtent.height);

    SDL_SetWindowMinimumSize(window.get(), 100, 100);
    SDL_SetWindowPosition(window.get(), 1, 31);

    auto vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
    if (!vkGetInstanceProcAddr)
        throw SDLException("Failed to get vkGetInstanceProcAddr");
    context.emplace(vkGetInstanceProcAddr);

    init_vulkan();

    _isInitialized = true;
}

void KitsuneEngine::cleanup() {
    SDL_Quit();
}

void KitsuneEngine::handleEvent(const SDL_Event* event) {
    switch (event->type) {
    case SDL_EVENT_QUIT:
        running = false;
        break;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        _windowExtent = get_window_extents();
        fmt::println("Window extent resized to: {}x{}", _windowExtent.width, _windowExtent.height);
        break;
    case SDL_EVENT_WINDOW_MINIMIZED:
        fmt::println("Window minimized: {}x{}", _windowExtent.width, _windowExtent.height);
        break;
    case SDL_EVENT_WINDOW_MAXIMIZED:
        fmt::println("Window maximized: {}x{}", _windowExtent.width, _windowExtent.height);

        break;
    case SDL_EVENT_WINDOW_RESTORED:
        fmt::println("Window restored: {}x{}", _windowExtent.width, _windowExtent.height);


        break;
    }
}

void KitsuneEngine::run() {
    SDL_ShowWindow(window.get());


    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(&event);
        }

        draw();
    }
}

void KitsuneEngine::draw() {}

void KitsuneEngine::init_vulkan() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createDevice();
    createCommandPool();
    createSwapchain();
    createFrameResources();
    createRenderPass();
    createFramebuffers();
    createSyncObjects();
    createGraphicsPipeline();
    createCommandBuffers();
}

void KitsuneEngine::createInstance() {
    vk::ApplicationInfo applicationInfo{};
    applicationInfo.setPApplicationName(ENGINE_NAME)
        .setApplicationVersion(ENGINE_VERSION)
        .setPEngineName(ENGINE_NAME)
        .setEngineVersion(ENGINE_VERSION)
        .setApiVersion(API_VERSION);

    uint32_t extensionCount;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (!extensions)
        throw SDLException("Failed to get Vulkan instance extensions");

    vk::InstanceCreateInfo createInfo{};
    createInfo.setPApplicationInfo(&applicationInfo)
        .setEnabledExtensionCount(extensionCount)
        .setPpEnabledExtensionNames(extensions);

    instance.emplace(*context, createInfo);
}

void KitsuneEngine::createSurface() {
    VkSurfaceKHR rawSurface;
    if (!SDL_Vulkan_CreateSurface(window.get(), **instance, nullptr, &rawSurface))
        throw SDLException("Failed to create surface");
    surface.emplace(*instance, rawSurface);
}

void KitsuneEngine::pickPhysicalDevice() {
    auto devices = instance->enumeratePhysicalDevices();
    for (const auto& device : devices) {
        auto [graphicsIndex, presentIndex] = is_device_full_support(device);
        if (graphicsIndex && presentIndex) {
            physicalDevice = device;
            msaaSamples = get_max_usable_sample_count(device);
            deviceIndices = { graphicsIndex, presentIndex };
            break;
        }
    }

    if (!physicalDevice) throw std::runtime_error("No suitable physical device found");

    auto const rawDeviceName{ physicalDevice->getProperties().deviceName };
    std::string deviceName(rawDeviceName.data(), std::strlen(rawDeviceName));
    fmt::println("{}", deviceName);
}

void KitsuneEngine::createDevice() {
    if (!physicalDevice || !deviceIndices.first || !deviceIndices.second)
        throw std::runtime_error("Cannot create device: No physical device or queue indices");

    std::set<uint32_t> uniqueQueueFamilies = { *deviceIndices.first, *deviceIndices.second };
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    for (uint32_t queueFamilyIndex : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.setQueueFamilyIndex(queueFamilyIndex)
            .setQueueCount(1)
            .setPQueuePriorities(&queuePriority);
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.setSamplerAnisotropy(true);


    vk::PhysicalDeviceVulkan13Features featuresV13{};
	featuresV13.setSynchronization2(true);



    vk::DeviceCreateInfo createInfo{};
    createInfo.setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
        .setPQueueCreateInfos(queueCreateInfos.data())
        .setEnabledExtensionCount(static_cast<uint32_t>(REQUIRED_EXTENSIONS.size()))
        .setPpEnabledExtensionNames(REQUIRED_EXTENSIONS.data())
        .setPEnabledFeatures(&deviceFeatures)
        .setPNext(&featuresV13);
    

    device.emplace(*physicalDevice, createInfo);
    graphicsQueue.emplace(*device, *deviceIndices.first, 0);
    presentQueue.emplace(*device, *deviceIndices.second, 0);

    fmt::println("Logical device created with graphics queue: {}, present queue: {}",
        *deviceIndices.first, *deviceIndices.second);
}

void KitsuneEngine::createCommandPool() {


    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.setQueueFamilyIndex(*deviceIndices.first)
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    commandPool.emplace(*device, poolInfo);
    fmt::println("Command pool created for graphics queue family: {}", *deviceIndices.first);
}

void KitsuneEngine::createSwapchain() {


    // Get surface capabilities, formats, and present modes
    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice->getSurfaceCapabilitiesKHR(*surface);
    std::vector<vk::SurfaceFormatKHR> formats = physicalDevice->getSurfaceFormatsKHR(*surface);
    std::vector<vk::PresentModeKHR> presentModes = physicalDevice->getSurfacePresentModesKHR(*surface);

    // Choose swapchain parameters
    vk::SurfaceFormatKHR surfaceFormat = choose_swap_format(formats);
    vk::PresentModeKHR presentMode = choose_present_mode(presentModes);
    vk::Extent2D extent = choose_swap_extent(capabilities);

    // Choose image count for double buffering
    uint32_t imageCount = capabilities.minImageCount; // Start with min
    if (capabilities.minImageCount == 1) {
        imageCount = 2; // Prefer 2 for double buffering if min is 1
    }
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount; // Clamp to max if needed
    }

    // Create swapchain
    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.setSurface(*surface)
        .setMinImageCount(imageCount)
        .setImageFormat(surfaceFormat.format)
        .setImageColorSpace(surfaceFormat.colorSpace)
        .setImageExtent(extent)
        .setImageArrayLayers(1) // 1 for standard 2D rendering
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment) // Render directly to swapchain images
        .setPreTransform(capabilities.currentTransform) // Use current transform (no rotation)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque) // Opaque composition
        .setPresentMode(presentMode)
        .setClipped(true); // Clip obscured pixels

    // Handle queue families
    std::array<uint32_t, 2> queueFamilyIndices = { *deviceIndices.first, *deviceIndices.second };
    if (*deviceIndices.first != *deviceIndices.second) {
        createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
            .setQueueFamilyIndexCount(2)
            .setPQueueFamilyIndices(queueFamilyIndices.data());
    }
    else {
        createInfo.setImageSharingMode(vk::SharingMode::eExclusive)
            .setQueueFamilyIndexCount(0)
            .setPQueueFamilyIndices(nullptr);
    }

    swapChain.emplace(*device, createInfo);
    swapChainImages = swapChain->getImages();
    swapChainImageFormat = surfaceFormat.format; // Cache format
    swapChainExtent = extent; // Cache extent
    fmt::println("Swapchain created with extent {}x{}, format {}, present mode {}, image count {}",
        swapChainExtent.width, swapChainExtent.height, vk::to_string(swapChainImageFormat),
        vk::to_string(presentMode), imageCount);
    fmt::println("Retrieved {} swapchain images", swapChainImages.size());
}

void KitsuneEngine::createImageViews()
{
    swapchainImageViews.reserve(swapChainImages.size());

    vk::ImageViewCreateInfo createInfo{}; createInfo
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(swapChainImageFormat)
        .setSubresourceRange(vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });


    for (size_t i = 0; i < swapChainImages.size(); i++) 
    {
        createInfo.setImage(swapChainImages[i]);
        swapchainImageViews.push_back({ *device, createInfo });
    }

    fmt::println("swapchain image views size:{}", swapchainImageViews.size());
}

void KitsuneEngine::createDepthResources()
{
    vk::Format depthFormat = findDepthFormat();


    createImage(swapChainExtent.width, swapChainExtent.height,1,msaaSamples, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);
    createImageView(*depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1, depthImageView);
}

void KitsuneEngine::createColorResources()
{
    vk::Format colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, colorImage, colorImageMemory);
    createImageView(*colorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1, colorImageView);
}

void KitsuneEngine::createFrameResources() 
{
    createImageViews();
    createDepthResources();
    createColorResources();
}

void KitsuneEngine::createRenderPass() {

    vk::SampleCountFlagBits msaaSamples = get_msaa_samples();

    // Multisampled color attachment (attachment 0)
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.setFormat(swapChainImageFormat)
        .setSamples(msaaSamples) // 4x MSAA
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare) // Transient, resolved to swapchain
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eAttachmentOptimalKHR);

    vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eAttachmentOptimalKHR };

    // Multisampled depth attachment (attachment 1)
    vk::Format depthFormat = findDepthFormat();
    vk::AttachmentDescription depthAttachment{};
    depthAttachment.setFormat(depthFormat)
        .setSamples(msaaSamples) // 4x MSAA
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference depthAttachmentRef{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };

    // Single-sampled resolve attachment (swapchain, attachment 2)
    vk::AttachmentDescription resolveAttachment{};
    resolveAttachment.setFormat(swapChainImageFormat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference resolveAttachmentRef{ 2, vk::ImageLayout::eAttachmentOptimalKHR };

    // Subpass with resolve
    vk::SubpassDescription subpass{};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(1)
        .setPColorAttachments(&colorAttachmentRef)
        .setPDepthStencilAttachment(&depthAttachmentRef)
        .setPResolveAttachments(&resolveAttachmentRef);

    vk::SubpassDependency dependency{};
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setSrcAccessMask(vk::AccessFlagBits::eNone)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    std::array<vk::AttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, resolveAttachment };
    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.setAttachmentCount(static_cast<uint32_t> (attachments.size()))
        .setPAttachments(attachments.data())
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
        .setDependencyCount(1)
        .setPDependencies(&dependency);

    renderPass.emplace(*device, renderPassInfo);
    fmt::println("Render pass created with multisampled color ({}), depth ({}), and resolve (swapchain: {})",
        vk::to_string(swapChainImageFormat), vk::to_string(depthFormat), vk::to_string(swapChainImageFormat));
}

void KitsuneEngine::createFramebuffers() {


    uint32_t imageCount = get_image_count();


    framebuffers.clear();
    framebuffers.reserve(imageCount);

    for (auto const& view : swapchainImageViews)
    {
        std::array<vk::ImageView, 3> attachments = { *colorImageView, *depthImageView, *view };

        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.setRenderPass(*renderPass)
            .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
            .setPAttachments(attachments.data())
            .setWidth(swapChainExtent.width)
            .setHeight(swapChainExtent.height)
            .setLayers(1);
        framebuffers.emplace_back(*device, framebufferInfo);
    }

    fmt::println("Created {} framebuffers with extent {}x{}", framebuffers.size(), swapChainExtent.width, swapChainExtent.height);
}

void KitsuneEngine::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, std::optional < vk::raii::Image>& image, std::optional < vk::raii::DeviceMemory>& imageMemory)
{
    vk::ImageCreateInfo imageInfo{}; imageInfo
        .setImageType(vk::ImageType::e2D)
        .setFormat(format)
        .setExtent({ width, height, 1 })
        .setMipLevels(mipLevels)
        .setArrayLayers(1)
        .setSamples(numSamples)
        .setTiling(tiling)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(vk::ImageLayout::eUndefined);

    image = device->createImage(imageInfo);

    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice->getMemoryProperties();
    vk::MemoryRequirements memoryRequirements = image->getMemoryRequirements();

    uint32_t typeBits = memoryRequirements.memoryTypeBits;
    uint32_t typeIndex = UINT32_MAX; // Using standard max value
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeBits & 1) &&
            ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
            typeIndex = i;
            break;
        }
        typeBits >>= 1;
    }

    if (typeIndex == UINT32_MAX) {
        throw std::runtime_error("Failed to find suitable memory type for image");
    }
 
    vk::MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size, typeIndex);

    imageMemory = device->allocateMemory(memoryAllocateInfo);

    image->bindMemory(*imageMemory, 0);

}

void KitsuneEngine::createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, std::optional<vk::raii::ImageView>& imageView)
{
    vk::ImageViewCreateInfo viewInfo{}; viewInfo
        .setImage(image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(format)
        .setSubresourceRange(vk::ImageSubresourceRange{ aspectFlags, 0, mipLevels, 0, 1 });

    imageView = device->createImageView(viewInfo);
}

void KitsuneEngine::createSyncObjects() 
{
    imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semaphoreInfo{};
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled); // Start signaled so first frame can proceed

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        imageAvailableSemaphores.emplace_back(*device, semaphoreInfo);
        renderFinishedSemaphores.emplace_back(*device, semaphoreInfo);
        inFlightFences.emplace_back(*device, fenceInfo);
    }

    fmt::println("Created sync objects for {} frames in flight", MAX_FRAMES_IN_FLIGHT);
}


void KitsuneEngine::recreate_swapchain()
{

}

void KitsuneEngine::createGraphicsPipeline() {
    vertShaderModule = createShaderModule("shaders/shader.vert.spv");
    fragShaderModule = createShaderModule("shaders/shader.frag.spv");

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
    vertexInputInfo.setVertexBindingDescriptionCount(0)
        .setVertexAttributeDescriptionCount(0);

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(false);

    vk::Viewport viewPort{};
    viewPort.setX(0)
        .setY(0)
        .setWidth((float)swapChainExtent.width)
        .setHeight((float)swapChainExtent.height)
        .setMinDepth(0.0f)
        .setMaxDepth(1.0f);

    vk::Rect2D scissor{};
    scissor.setOffset({ 0,0 })
        .setExtent(swapChainExtent);

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.setViewportCount(1)
        .setScissorCount(1)
        .setPViewports(&viewPort)
        .setPScissors(&scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.setDepthClampEnable(false)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0f)
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eClockwise);

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.setSampleShadingEnable(false)
        .setRasterizationSamples(msaaSamples);

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
        .setBlendEnable(false);

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.setLogicOpEnable(false)
        .setAttachmentCount(1)
        .setPAttachments(&colorBlendAttachment);

    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.setDepthTestEnable(true)
        .setDepthWriteEnable(true)
        .setDepthCompareOp(vk::CompareOp::eLess);

    std::array dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.setDynamicStateCount(dynamicStates.size())
        .setPDynamicStates(dynamicStates.data());

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayout = device->createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.setStageCount(stages.size())
        .setPStages(stages.data())
        .setPVertexInputState(&vertexInputInfo)
        .setPInputAssemblyState(&inputAssembly)
        .setPViewportState(&viewportState)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multisampling)
        .setPDepthStencilState(&depthStencil)
        .setPColorBlendState(&colorBlending)
        .setPDynamicState(&dynamicState)
        .setLayout(*pipelineLayout)
        .setRenderPass(*renderPass)
        .setSubpass(0);

    graphicsPipeline = device->createGraphicsPipeline(nullptr, pipelineInfo);
    fmt::println("Graphics pipeline created");
}

void KitsuneEngine::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.setCommandPool(*commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(MAX_FRAMES_IN_FLIGHT);

    commandBuffers = device->allocateCommandBuffers(allocInfo);

    fmt::println("{} commandBuffer is allocated", static_cast<uint32_t>(commandBuffers.size()));
}

vk::Extent2D KitsuneEngine::get_window_extents() const {
    int w, h;
    SDL_GetWindowSizeInPixels(window.get(), &w, &h);
    return vk::Extent2D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
}

vk::Extent2D KitsuneEngine::get_monitor_extents() const {
    int displayCount;
    SDL_DisplayID* displays = SDL_GetDisplays(&displayCount);
    if (!displays || displayCount <= 0) {
        fmt::println("Warning: Failed to get display list, using fallback 1280x720: {}", SDL_GetError());
        SDL_free(displays);
        return vk::Extent2D{ 1280, 720 };
    }

    // Use the first display (typically primary)
    SDL_DisplayID primaryDisplay = displays[0];
    SDL_free(displays); // Free the array after use

    const SDL_DisplayMode* displayMode = SDL_GetCurrentDisplayMode(primaryDisplay);
    if (displayMode) {
        return vk::Extent2D{ static_cast<uint32_t>(displayMode->w), static_cast<uint32_t>(displayMode->h) };
    }
    else {
        fmt::println("Warning: Failed to get monitor size for display {}, using fallback 1280x720: {}", primaryDisplay, SDL_GetError());
        return vk::Extent2D{ 1280, 720 };
    }
}

vk::PresentModeKHR KitsuneEngine::choose_present_mode(const std::vector<vk::PresentModeKHR>& availableModes) const {
    if (vsync) {
        if (std::find(availableModes.begin(), availableModes.end(), vk::PresentModeKHR::eMailbox) != availableModes.end()) {
            return vk::PresentModeKHR::eMailbox;
        }
        return vk::PresentModeKHR::eFifo;
    }
    else {
        if (std::find(availableModes.begin(), availableModes.end(), vk::PresentModeKHR::eImmediate) != availableModes.end()) {
            return vk::PresentModeKHR::eImmediate;
        }
        return vk::PresentModeKHR::eFifo;
    }
}

vk::Extent2D KitsuneEngine::choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        vk::Extent2D extent = get_window_extents();
        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return extent;
    }
}

vk::SurfaceFormatKHR KitsuneEngine::choose_swap_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const {
    for (const auto& format : availableFormats) {
        if (format.format == vk::Format::eB8G8R8A8Srgb &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }
    if (!availableFormats.empty()) {
        fmt::println("Warning: Preferred format (B8G8R8A8_SRGB, SRGB_NONLINEAR) not found, using first available format");
        return availableFormats[0];
    }
    throw std::runtime_error("No surface formats available");
}

std::optional<uint32_t> KitsuneEngine::find_graphics_queue_family_index(const std::vector<vk::QueueFamilyProperties>& queueFamilyProperties) {
    auto graphicsQueueFamilyProperty = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
        [](vk::QueueFamilyProperties qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; });
    if (graphicsQueueFamilyProperty != queueFamilyProperties.end()) {
        return static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
    }
    return std::nullopt;
}

std::pair<std::optional<uint32_t>, std::optional<uint32_t>>
KitsuneEngine::find_graphics_and_present_queue_family_index(const vk::raii::PhysicalDevice& physicalDevice) {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    auto graphicsIndexOpt = find_graphics_queue_family_index(queueFamilyProperties);
    if (!graphicsIndexOpt) {
        return { std::nullopt, std::nullopt };
    }
    if (physicalDevice.getSurfaceSupportKHR(*graphicsIndexOpt, *surface)) {
        return { *graphicsIndexOpt, *graphicsIndexOpt };
    }
    uint32_t i = 0;
    std::optional<uint32_t> presentIndex;
    for (const auto& queueFamily : queueFamilyProperties) {
        if (i != *graphicsIndexOpt) {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics &&
                physicalDevice.getSurfaceSupportKHR(i, *surface)) {
                return { i, i };
            }
            if (!presentIndex && physicalDevice.getSurfaceSupportKHR(i, *surface)) {
                presentIndex = i;
            }
        }
        i++;
    }
    return { *graphicsIndexOpt, presentIndex };
}

bool KitsuneEngine::check_device_extensions_support(const vk::raii::PhysicalDevice& device) {
    auto availableExtensions = device.enumerateDeviceExtensionProperties();
    for (const char* requiredExt : REQUIRED_EXTENSIONS) {
        bool found = false;
        for (const auto& availableExt : availableExtensions) {
            if (std::strcmp(requiredExt, availableExt.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

std::pair<std::optional<uint32_t>, std::optional<uint32_t>>
KitsuneEngine::is_device_full_support(const vk::raii::PhysicalDevice& physicalDevice) {
    auto [graphicsIndex, presentIndex] = find_graphics_and_present_queue_family_index(physicalDevice);
    if (!graphicsIndex || !presentIndex) return { std::nullopt, std::nullopt };

    if (!check_device_extensions_support(physicalDevice)) return { std::nullopt, std::nullopt };

    auto const formats = physicalDevice.getSurfaceFormatsKHR(*surface);
    auto const presentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

    if (formats.empty() || presentModes.empty()) return { std::nullopt, std::nullopt };

    auto supportedFeatures = physicalDevice.getFeatures();
    if (!supportedFeatures.samplerAnisotropy) return { std::nullopt, std::nullopt };

    return { graphicsIndex, presentIndex };
}

vk::Format KitsuneEngine::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const {
    if (!physicalDevice) {
        throw std::runtime_error("Cannot find supported format: Physical device not initialized");
    }

    for (const auto& format : candidates) {
        vk::FormatProperties props = physicalDevice->getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format!");
}

vk::Format KitsuneEngine::findDepthFormat() const {
    std::vector<vk::Format> candidates = { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };
    return findSupportedFormat(candidates, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::SampleCountFlagBits KitsuneEngine::get_max_usable_sample_count(const vk::raii::PhysicalDevice& device) const {
    vk::PhysicalDeviceProperties properties = device.getProperties();
    vk::SampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

    if (counts & vk::SampleCountFlagBits::e64) return vk::SampleCountFlagBits::e64;
    if (counts & vk::SampleCountFlagBits::e32) return vk::SampleCountFlagBits::e32;
    if (counts & vk::SampleCountFlagBits::e16) return vk::SampleCountFlagBits::e16;
    if (counts & vk::SampleCountFlagBits::e8) return vk::SampleCountFlagBits::e8;
    if (counts & vk::SampleCountFlagBits::e4) return vk::SampleCountFlagBits::e4;
    if (counts & vk::SampleCountFlagBits::e2) return vk::SampleCountFlagBits::e2;
    return vk::SampleCountFlagBits::e1;
}



std::vector<char> KitsuneEngine::readFile(const std::string& filename) const {

    // Get the base path of the executable
    const char* rawBasePath = SDL_GetBasePath();
    std::string fullPath;
    if (rawBasePath) {
        fullPath = std::string(rawBasePath) + filename;
    }
    else {
        fmt::println("Warning: SDL_GetBasePath failed, using relative path");
        fullPath = "./" + filename; // Fallback
    }
    std::ifstream file(fullPath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: {}", fullPath));
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

vk::raii::ShaderModule KitsuneEngine::createShaderModule(const std::string& filename) const
{
    auto sourceCode = readFile(filename);

    vk::ShaderModuleCreateInfo createInfo{}; createInfo
        .setCodeSize(sourceCode.size())
        .setPCode(reinterpret_cast<const uint32_t*>(sourceCode.data()));

    return device->createShaderModule(createInfo);
}

