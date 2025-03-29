#pragma once
#include <kitsune_types.h>

class KitsuneEngine {
    class SDLException : std::runtime_error {
    public:
        explicit SDLException(const std::string& message)
            : std::runtime_error(fmt::format("{}: {}", message, SDL_GetError())) {}
    };

    static inline constexpr const char* ENGINE_NAME{ "Kitsune" };
    static inline constexpr auto ENGINE_VERSION{ vk::makeApiVersion(1, 0, 0, 0) };
    static inline constexpr uint32_t API_VERSION = VK_API_VERSION_1_4;
    static inline constexpr std::array<const char* const, 2> REQUIRED_EXTENSIONS = { vk::KHRSwapchainExtensionName, vk::KHRSynchronization2ExtensionName };
    static inline constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

public:
    KitsuneEngine() = default;
    ~KitsuneEngine() = default;

    void init();
    void run();
    void cleanup();

    SDL_Window* get_window() const { return window.get(); }
    const vk::raii::Context& get_context() const { if (!context) throw std::runtime_error("Context not initialized"); return *context; }
    const vk::raii::Instance& get_instance() const { if (!instance) throw std::runtime_error("Instance not initialized"); return *instance; }
    const vk::raii::SurfaceKHR& get_surface() const { if (!surface) throw std::runtime_error("Surface not initialized"); return *surface; }
    const vk::raii::PhysicalDevice& get_physical() const { if (!physicalDevice) throw std::runtime_error("Physical device not initialized"); return *physicalDevice; }
    const vk::raii::Device& get_device() const { if (!device) throw std::runtime_error("Device not initialized"); return *device; }
    const vk::raii::CommandPool& get_command_pool() const { if (!commandPool) throw std::runtime_error("Command pool not initialized"); return *commandPool; }
    const vk::raii::Queue& get_graphics_queue() const { if (!graphicsQueue) throw std::runtime_error("Graphics queue not initialized"); return *graphicsQueue; }
    const vk::raii::Queue& get_present_queue() const { if (!presentQueue) throw std::runtime_error("Present queue not initialized"); return *presentQueue; }
    const std::vector<vk::Image>& get_swapchain_images() const { if (swapChainImages.empty()) throw std::runtime_error("Swapchain images not initialized"); return swapChainImages; }
    const std::vector<vk::raii::ImageView>& get_swapchain_image_views() const { if (swapchainImageViews.empty()) throw std::runtime_error("Swapchain image views not initialized"); return swapchainImageViews; }
    const std::pair<std::optional<uint32_t>, std::optional<uint32_t>>& get_device_indices() const { return deviceIndices; }
    vk::Extent2D get_swapchain_extent() const { return swapChainExtent; }
    vk::Format get_swapchain_image_format() const { return swapChainImageFormat; }
    const vk::raii::RenderPass& get_render_pass() const { if (!renderPass) throw std::runtime_error("Render pass not initialized"); return *renderPass; }
    const std::vector<vk::raii::Framebuffer>& get_framebuffers() const { if (framebuffers.empty()) throw std::runtime_error("Framebuffers not initialized"); return framebuffers; }
    vk::SampleCountFlagBits get_msaa_samples() const { return msaaSamples; }

    bool is_initialized() const { return _isInitialized; }
    bool is_running() const { return running; }
    bool is_stopped() const { return stop_rendering; }
    int frame_number() const { return _frameNumber; }
    VkExtent2D window_extent() const { return _windowExtent; }

    uint32_t get_image_count() const { return static_cast<uint32_t>(swapchainImageViews.size()); }
    vk::Format findDepthFormat() const;

private:
    bool _isInitialized{ false };
    bool running{ true };
    bool stop_rendering{ false };
    int _frameNumber{ 0 };
    VkExtent2D _windowExtent{ 800, 600 }; // Still here as a member, but not used for initial window size
    bool vsync{ true };
    bool setDefaulWinSize{ true };
    bool framebufferResized{ false };    // New: Tracks if framebuffer needs recreation
    std::pair<vk::Result, uint32_t> currentImageIndex{ vk::Result::eSuccess, 0 };     // New: Current swapchain image index
    int currentFrameIndex{ 0 };          // New: Current frame-in-flight index
    bool isFrameStarted{ false };        // New: Tracks if frame is in progress


    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{ nullptr, SDL_DestroyWindow };
    std::optional<vk::raii::Context> context{};
    std::optional<vk::raii::Instance> instance{};
    std::optional<vk::raii::SurfaceKHR> surface{};
    std::pair<std::optional<uint32_t>, std::optional<uint32_t>> deviceIndices{ std::nullopt, std::nullopt };
    std::optional<vk::raii::PhysicalDevice> physicalDevice{};
    std::optional<vk::raii::Device> device{};
    std::optional<vk::raii::CommandPool> commandPool{};
    std::optional<vk::raii::Queue> graphicsQueue{};
    std::optional<vk::raii::Queue> presentQueue{};
    std::optional<vk::raii::SwapchainKHR> swapChain{};
    std::vector<vk::Image> swapChainImages{};
    std::vector<vk::raii::ImageView> swapchainImageViews{};
    vk::Extent2D swapChainExtent{ 0, 0 };
    vk::Format swapChainImageFormat{ vk::Format::eUndefined };
    std::optional<vk::raii::RenderPass> renderPass{};
    std::optional<vk::raii::Image> colorImage{}; // Changed to vector for multiple images
    std::optional<vk::raii::DeviceMemory> colorImageMemory{}; // Vector for memory
    std::optional<vk::raii::ImageView> colorImageView{}; // Vector for views
    std::optional<vk::raii::Image> depthImage{}; // Vector for multiple depth images
    std::optional<vk::raii::DeviceMemory> depthImageMemory{};
    std::optional<vk::raii::ImageView> depthImageView{};
    std::vector<vk::raii::Framebuffer> framebuffers{};
    vk::SampleCountFlagBits msaaSamples{ vk::SampleCountFlagBits::e1 };
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores; // New: Sync for swapchain image availability
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores; // New: Sync for render completion
    std::vector<vk::raii::Fence> inFlightFences;              // New: Sync for frame completion
    std::vector<vk::raii::CommandBuffer> commandBuffers;
    // New: Shader and pipeline members
    std::optional<vk::raii::ShaderModule> vertShaderModule{};
    std::optional<vk::raii::ShaderModule> fragShaderModule{};
    std::optional<vk::raii::PipelineLayout> pipelineLayout{};
    std::optional<vk::raii::Pipeline> graphicsPipeline{};

    void draw();
    void handleEvent(const SDL_Event* event);

    void init_vulkan();
    void create_instance();
    void create_surface();
    void pick_physical_device();
    void create_device();
    void create_command_pool();
    void create_swapchain();
    void createImageViews();
    void createDepthResources();
    void createColorResources();
    void create_frame_resources();
    void create_render_pass();
    void create_framebuffers();
    void createSyncObjects();
    void recreate_swapchain();
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, std::optional < vk::raii::Image>& image, std::optional < vk::raii::DeviceMemory>& imageMemory);
    void createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels, std::optional<vk::raii::ImageView>& imageView);
    
    void createGraphicsPipeline(); // New: Pipeline setup
    void recordCommandBuffer(vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex); // New: Record draw commands

    vk::Extent2D get_window_extents() const;
    vk::Extent2D get_monitor_extents() const; // New helper for monitor size
    vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& availableModes) const;
    vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
    vk::SurfaceFormatKHR choose_swap_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;

    std::optional<uint32_t> find_graphics_queue_family_index(const std::vector<vk::QueueFamilyProperties>& queueFamilyProperties);
    std::pair<std::optional<uint32_t>, std::optional<uint32_t>>
        find_graphics_and_present_queue_family_index(const vk::raii::PhysicalDevice& physicalDevice);
    bool check_device_extensions_support(const vk::raii::PhysicalDevice& device);
    std::pair<std::optional<uint32_t>, std::optional<uint32_t>> is_device_full_support(const vk::raii::PhysicalDevice& device);
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const;
    vk::SampleCountFlagBits get_max_usable_sample_count(const vk::raii::PhysicalDevice& device) const;
    vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
    std::vector<char> readFile(const std::string& filename); // New: Load SPIR-V
};