#pragma once

#include <vulkan\vulkan.h>
#include <vector>
#include <GLFW\glfw3.h>

class Vulkan
{
private:
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkSurfaceFormatKHR surfaceFormat;
	VkExtent2D surfaceExtent;

	std::vector<VkImage> swapchainImages;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> graphicsCommandBuffers;

	VkRenderPass renderPass;
	VkPipeline graphicsPipeline;

	VkSemaphore imageIsAvailable;
	VkSemaphore imageIsRendered;

	uint32_t graphicsFamilyIndex = -1;
	VkQueue graphicsQueue;

public:
	static Vulkan app;

	Vulkan();
	virtual ~Vulkan();
	
	void createSurface(GLFWwindow* window);
	void init();
	void draw();

private:
	void createInstance();
	void createDevice();
	uint32_t chooseQueueFamilyIndex();
	void createSwapchain();
	void createCommandBuffers();
	void recordDrawCommand();
	void createRenderPass();
	void createGraphicsPipeline();
	VkPipelineShaderStageCreateInfo createShaderStage(const std::string& filename, VkShaderStageFlagBits shaderStage);
};
