




#pragma once
#include <kitsune_types.h>
#include <kitsune_windowing.hpp>
	struct PerFrame
	{
		std::optional <vk::raii::Fence>          queue_submit_fence{ VK_NULL_HANDLE };
		std::optional < vk::raii::CommandPool>   primary_command_pool{ VK_NULL_HANDLE };
		std::optional < vk::raii::CommandBuffer> primary_command_buffer{ VK_NULL_HANDLE };
		std::optional < vk::raii::Semaphore>     swapchain_acquire_semaphore{ VK_NULL_HANDLE };
		std::optional < vk::raii::Semaphore>     swapchain_release_semaphore{ VK_NULL_HANDLE };
	};

	struct VulkanResorces
	{
		std::optional <vk::raii::Context> context{ VK_NULL_HANDLE };
		std::optional <vk::raii::Instance> instance{ VK_NULL_HANDLE };
		std::optional <vk::raii::PhysicalDevice> physicalDevice{ VK_NULL_HANDLE };
		std::optional <vk::raii::Device> device{ VK_NULL_HANDLE };
		std::optional <vk::raii::SurfaceKHR> surface{ VK_NULL_HANDLE };
		std::optional <vk::raii::SwapchainKHR> swapchain{ VK_NULL_HANDLE };
		std::optional <vk::raii::PipelineLayout> pipelineLayout{ VK_NULL_HANDLE };
		std::optional <vk::raii::Pipeline> graphicsPipeline{ VK_NULL_HANDLE };
		std::optional <vk::raii::CommandPool> commandPool{ VK_NULL_HANDLE };
	};


	class KitsuneEngine {
	public:
		KitsuneEngine(KitsuneWindowing& windowing);
		~KitsuneEngine();

		void Init();
		void run();

		std::vector<const char*> GetRequiredInstanceExtensions(const std::vector<vk::ExtensionProperties>& available);
		bool AreExtensionsSupported(const std::vector<const char*>& required, const std::vector<vk::ExtensionProperties>& available) const;

	private:
		bool isRunning{ false };
		bool hasPortability{ false };
		
		KitsuneWindowing& windowing_;
		VulkanResorces resorces{};
		std::string basePath;
		vk::Extent2D windowExtent{ 800, 600 };



		void CreateContext();
		void CreateInstance();
	};


