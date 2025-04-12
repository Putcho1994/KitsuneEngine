#pragma once
#include <kitsune_types.h>
#include <kitsune_windowing.hpp>


struct PerFrame
{
	std::optional <vk::raii::Fence>          queue_submit_fence{};
	std::optional < vk::raii::CommandPool>   primary_command_pool{};
	std::optional < vk::raii::CommandBuffer> primary_command_buffer{};
	std::optional < vk::raii::Semaphore>     swapchain_acquire_semaphore{};
	std::optional < vk::raii::Semaphore>     swapchain_release_semaphore{};
};


struct VulkanResorces
{
	std::optional <vk::raii::Context> context{};
	std::optional <vk::raii::Instance> instance{};
	std::optional <vk::raii::PhysicalDevice> physicalDevice{};
	std::optional <vk::raii::Device> device{};
	std::optional <vk::raii::SurfaceKHR> surface{};
	std::optional <vk::raii::SwapchainKHR> swapchain{};
	std::optional <vk::raii::PipelineLayout> pipelineLayout{};
	std::optional <vk::raii::Pipeline> graphicsPipeline{};
	std::optional <vk::raii::CommandPool> commandPool{};

	std::optional<vk::raii::Queue> graphicsQueue{};
    std::optional<vk::raii::Queue> presentQueue{};
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphics;
	std::optional<uint32_t> present;
};

class KitsuneEngine
{
public:
	KitsuneEngine(KitsuneWindowing& windowing);
	~KitsuneEngine();

	void Init();
	void run();

	std::vector<const char*> GetRequiredInstanceExtensions(const std::vector<vk::ExtensionProperties>& available);
	bool AreExtensionsSupported(const std::vector<const char*>& required, const std::vector<vk::ExtensionProperties>& available) const;
	VulkanResorces resorces{};

	const std::string& GetBasePath() const { return basePath; };
	const vk::Extent2D& GetWindowExtent() const { return windowExtent; };
	const QueueFamilyIndices& GetQueueFamilyIndices() const { return queueFamilyIndices; };

	void ResetWindowExtent();

	void WaitForIdle() const
	{
		if (resorces.device) {
			resorces.device->waitIdle();
		}
	}

private:
	bool isRunning{ false };
	bool hasPortability{ false };

	KitsuneWindowing& windowing_;

	std::string basePath;
	vk::Extent2D windowExtent{ 800, 600 };

	QueueFamilyIndices queueFamilyIndices{ std::nullopt, std::nullopt };

	void CreateContext();
	void CreateInstance();
	void CreateSurface();
	void SelectPhysicalDevice();
	void CreateLogicalDevice();

	QueueFamilyIndices FindQueueFamilies(const vk::raii::PhysicalDevice& device) const;
};


