#include "Vulkan.h"
#include <iostream>
#include <assert.h>

Vulkan Vulkan::app;

Vulkan::Vulkan()
{
	createInstance();
	createDevice();
}

void Vulkan::createInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pApplicationName = "Vulkan Application";
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

	// These extensions are required to display something on the screen
	const char* extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
	// For debugging
	const char* layers[] = {"VK_LAYER_LUNARG_standard_validation"};

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.enabledExtensionCount = 2;
	instanceInfo.ppEnabledExtensionNames = extensions;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledLayerCount = 1;
	instanceInfo.ppEnabledLayerNames = layers;
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

	VkResult res = vkCreateInstance(&instanceInfo, nullptr, &instance);
	assert(res == VK_SUCCESS);
}

void Vulkan::createDevice()
{
	uint32_t physicalDeviceCount;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

	physicalDevice = physicalDevices[0];
	
	// This extension is required to display something on the screen
	const char* extensions[] = { "VK_KHR_swapchain" };

	float queuePriorities = { 0.0f };
	VkDeviceQueueCreateInfo queueInfo = {};
	queueInfo.pQueuePriorities = &queuePriorities;
	queueInfo.queueCount = 1;
	queueInfo.queueFamilyIndex = chooseQueueFamilyIndex();
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.enabledExtensionCount = 1;
	deviceInfo.ppEnabledExtensionNames = extensions;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	VkResult res = vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device);
	assert(res == VK_SUCCESS);
}

uint32_t Vulkan::chooseQueueFamilyIndex()
{
	uint32_t familyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> familyProperties(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, familyProperties.data());

	for (uint32_t i = 0; i < familyCount; i++) {
		// This family supports graphics
		if (familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT == VK_QUEUE_GRAPHICS_BIT) {
			graphicsFamilyIndex = i;
			break;
		}
	}

	return graphicsFamilyIndex;
}

void Vulkan::init()
{
	createSwapchain();
	createCommandBuffers();
	recordDrawCommand();
}

void Vulkan::createSwapchain()
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

	uint32_t formatsCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, surfaceFormats.data());

	uint32_t presentModesCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModesCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, presentModes.data());
	
	VkSwapchainCreateInfoKHR swapchainInfo = {};
	swapchainInfo.clipped = true;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.queueFamilyIndexCount = 1;
	swapchainInfo.pQueueFamilyIndices = &graphicsFamilyIndex;
	swapchainInfo.surface = surface;
	swapchainInfo.imageFormat = surfaceFormats[0].format; // TODO: Choose a better one
	swapchainInfo.imageColorSpace = surfaceFormats[0].colorSpace; // TODO: Choose a better one
	swapchainInfo.imageExtent = surfaceCapabilities.currentExtent;
	swapchainInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
	swapchainInfo.presentMode = presentModes[0]; // TODO : Choose a better one
	swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
	swapchainInfo.oldSwapchain = swapchain;
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

	VkResult res = vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain);
	assert(res == VK_SUCCESS);

	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);
	swapchainImages.resize(swapchainImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());
}

void Vulkan::createSurface(GLFWwindow* window)
{
	glfwCreateWindowSurface(instance, window, nullptr, &surface);
}

void Vulkan::createCommandBuffers()
{
	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.queueFamilyIndex = graphicsFamilyIndex;
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	VkResult res = vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool);
	assert(res == VK_SUCCESS);

	// One buffer per image on the swapchain

	VkCommandBufferAllocateInfo bufferAllocInfo = {};
	bufferAllocInfo.commandBufferCount = swapchainImages.size();
	bufferAllocInfo.commandPool = commandPool;
	bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	graphicsCommandBuffers.resize(swapchainImages.size());
	res = vkAllocateCommandBuffers(device, &bufferAllocInfo, graphicsCommandBuffers.data());
	assert(res == VK_SUCCESS);
}

void Vulkan::recordDrawCommand()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1;

	// To change image layout
	VkImageMemoryBarrier barrierFromPresentToClear = {};
	barrierFromPresentToClear.subresourceRange = subResourceRange;
	barrierFromPresentToClear.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrierFromPresentToClear.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrierFromPresentToClear.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrierFromPresentToClear.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrierFromPresentToClear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	
	VkImageMemoryBarrier barrierFromClearToPresent = {};
	barrierFromClearToPresent.subresourceRange = subResourceRange;
	barrierFromClearToPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrierFromClearToPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrierFromClearToPresent.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrierFromClearToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrierFromClearToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	VkClearColorValue clearColor = {
		{0.0f, 0.0f, 0.0f, 1.0f}
	};

	for (uint32_t i = 0; i < swapchainImages.size(); i++) {
		barrierFromClearToPresent.image = swapchainImages[i];
		barrierFromPresentToClear.image = swapchainImages[i];

		vkBeginCommandBuffer(graphicsCommandBuffers[i], &beginInfo);
		vkCmdPipelineBarrier(graphicsCommandBuffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromPresentToClear);

		vkCmdClearColorImage(graphicsCommandBuffers[i], swapchainImages[i],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &subResourceRange);
		
		vkCmdPipelineBarrier(graphicsCommandBuffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromClearToPresent);
		vkEndCommandBuffer(graphicsCommandBuffers[i]);
	}
}

Vulkan::~Vulkan()
{
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
}
