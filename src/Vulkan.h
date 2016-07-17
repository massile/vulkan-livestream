#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#define VERTEX_BINDING_ID 0

struct Vertex {
	float position[3];
	float color[3];
};

// JUST FOR TESTING
struct Uniforms {
	glm::mat4 projectionMatrix;
	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
};

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
	std::vector<VkImageView> swapchainImageViews;
	VkFormat depthFormat;


	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> graphicsCommandBuffers;
	std::vector<VkFramebuffer> frameBuffers;

	VkRenderPass renderPass;
	VkPipeline graphicsPipeline;

	VkSemaphore imageIsAvailable;
	VkSemaphore imageIsRendered;

	VkPipelineLayout pipelineLayout;
	VkShaderModule vertexShaderModule;
	VkShaderModule fragmentShaderModule;

	VkDescriptorPool descriptorPool;


	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	Uniforms uniforms; // FOR TESTING
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformMemory;
	VkDescriptorBufferInfo uniformDescriptor;

	uint32_t graphicsFamilyIndex = -1;
	VkQueue graphicsQueue;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexMemory;

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
	void createSwapchainImageViews();

	void findCompatibleDepthFormat();
	
	void prepareVertices();
	void prepareUniforms();
	void loadUniforms();

	uint32_t getMemoryType(uint32_t typeBits, VkFlags properties);
	void createDescriptorPool();
	void setupDescriptorSets();

	void createCommandBuffers();
	void recordDrawCommand();
	void createRenderPass();
	void createGraphicsPipeline();
	void createFrameBuffers();
	VkPipelineShaderStageCreateInfo createShaderStage(const std::string& filename, VkShaderStageFlagBits shaderStage);
};
