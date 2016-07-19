#include "Vulkan.h"
#include <iostream>
#include <assert.h>
#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "helpers\Helpers.h" // TEMPORARY


Vulkan Vulkan::app;

Vulkan::Vulkan()
{
	createInstance();
	createDevice();

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageIsAvailable);
	vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageIsRendered);
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

	vkGetDeviceQueue(device, queueInfo.queueFamilyIndex, 0, &graphicsQueue);
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

	loadTexture("textures/test.jpg");
	loadSampler();

	findCompatibleDepthFormat();
	createDepthBuffer();

	prepareVertices();
	prepareUniforms();

	createRenderPass();
	createFrameBuffers();
	createGraphicsPipeline();
	recordDrawCommand();
}

void Vulkan::draw()
{
	loadUniforms();

	// Get next image in swapchain
	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageIsAvailable, 0, &imageIndex);

	VkPipelineShaderStageCreateFlags waitDstStageMsk = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &graphicsCommandBuffers[imageIndex]; // We want to send on the ith swapchain
	submitInfo.pWaitDstStageMask = &waitDstStageMsk;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageIsAvailable;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &imageIsRendered;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// Sends the draw command to the GPU (draws in the buffers)
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, 0);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &imageIsRendered;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	// Present the buffer's content to the screen (surface)
	vkQueuePresentKHR(graphicsQueue, &presentInfo);
}

void Vulkan::createSwapchain()
{
	VkBool32 surfaceSupported = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsFamilyIndex, surface, &surfaceSupported);
	assert(surfaceSupported == VK_TRUE);

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

	uint32_t formatsCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, surfaceFormats.data());

	this->surfaceFormat = surfaceFormats[0];

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

	surfaceExtent = swapchainInfo.imageExtent;
	VkResult res = vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain);
	assert(res == VK_SUCCESS);

	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);
	swapchainImages.resize(swapchainImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());

	createSwapchainImageViews();
}

void Vulkan::createSwapchainImageViews()
{
	swapchainImageViews.resize(swapchainImages.size());

	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.layerCount = 1;
	subResourceRange.levelCount = 1;

	VkImageViewCreateInfo imageViewInfo = {};
	imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewInfo.format = surfaceFormat.format;
	imageViewInfo.subresourceRange = subResourceRange;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

	VkResult res;
	for (int i = 0; i < swapchainImageViews.size(); i++) {
		imageViewInfo.image = swapchainImages[i];
		res = vkCreateImageView(device, &imageViewInfo, nullptr, &swapchainImageViews[i]);
		assert(res == VK_SUCCESS);
	}
}



void Vulkan::findCompatibleDepthFormat()
{
	VkFormat orderedDepthFormatsList[] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM,
	};

	VkFormatProperties props;
	for (VkFormat& format : orderedDepthFormatsList) {
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
		if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			depthFormat = format;
			return;
		}
	}
}

void Vulkan::createDepthBuffer()
{
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.extent.width = surfaceExtent.width;
	imageCreateInfo.extent.height = surfaceExtent.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.format = depthFormat;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

	VkResult res = vkCreateImage(device, &imageCreateInfo, nullptr, &depthBufferImage);
	assert(res == VK_SUCCESS);

	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.layerCount = 1;
	subResourceRange.levelCount = 1;

	VkImageViewCreateInfo depthImageViewCreateInfo = {};
	depthImageViewCreateInfo.format = depthFormat;
	depthImageViewCreateInfo.image = depthBufferImage;
	depthImageViewCreateInfo.subresourceRange = subResourceRange;
	depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

	// MEMORY ALLOCATION IS REQUIRED
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device, depthBufferImage, &memReqs);

	VkMemoryAllocateInfo allocateInfo = {};
	allocateInfo.allocationSize = memReqs.size;
	allocateInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	res = vkAllocateMemory(device, &allocateInfo, nullptr, &depthBufferMemory);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(device, depthBufferImage, depthBufferMemory, 0);
	assert(res == VK_SUCCESS);

	res = vkCreateImageView(device, &depthImageViewCreateInfo, nullptr, &depthBufferImageView);
	assert(res == VK_SUCCESS);
}

// TODO : Factorize this
void Vulkan::prepareVertices()
{
	std::vector<Vertex> vertices = {
		{ { 1.0f, -1.0f, -1.0f },{ 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
		{ { 1.0f,  1.0f, -1.0f },{ 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
		{ { 1.0f, -1.0f,  1.0f },{ 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
		{ { 1.0f,  1.0f,  1.0f },{ 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
	};

	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.size = vertices.size() * sizeof(Vertex);
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	VkResult res = vkCreateBuffer(device, &vertexBufferInfo, nullptr, &vertexBuffer);
	assert(res == VK_SUCCESS);

	// ALLOCATE MEMORY FOR VERTEX BUFFER

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, vertexBuffer, &memoryRequirements);


	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.allocationSize = memoryRequirements.size;
	// TODO : Copy this to the GPU
	memoryAllocInfo.memoryTypeIndex = getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	res = vkAllocateMemory(device, &memoryAllocInfo, nullptr, &vertexMemory);
	assert(res == VK_SUCCESS);

	void* data;
	res = vkMapMemory(device, vertexMemory, 0, memoryAllocInfo.allocationSize, 0, &data);
	assert(res == VK_SUCCESS);
	memcpy(data, vertices.data(), vertexBufferInfo.size);
	vkUnmapMemory(device, vertexMemory);

	res = vkBindBufferMemory(device, vertexBuffer, vertexMemory, 0);
	assert(res == VK_SUCCESS);

	/////////////////////////////////////////////////////////////////////////////

	std::vector<uint32_t> indices = {
		0, 2, 1,
		1, 2, 3
	};

	VkBufferCreateInfo indexBufferInfo = {};
	indexBufferInfo.size = indices.size() * sizeof(uint32_t);
	indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	res = vkCreateBuffer(device, &indexBufferInfo, nullptr, &indexBuffer);
	assert(res == VK_SUCCESS);

	// ALLOCATE MEMORY FOR INDEX BUFFER
	vkGetBufferMemoryRequirements(device, indexBuffer, &memoryRequirements);

	memoryAllocInfo.allocationSize = memoryRequirements.size;
	memoryAllocInfo.memoryTypeIndex = getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	res = vkAllocateMemory(device, &memoryAllocInfo, nullptr, &indexMemory);
	assert(res == VK_SUCCESS);

	res = vkMapMemory(device, indexMemory, 0, memoryAllocInfo.allocationSize, 0, &data);
	assert(res == VK_SUCCESS);
	memcpy(data, indices.data(), indexBufferInfo.size);
	vkUnmapMemory(device, indexMemory);

	res = vkBindBufferMemory(device, indexBuffer, indexMemory, 0);
	assert(res == VK_SUCCESS);

}

void Vulkan::prepareUniforms()
{
	VkBufferCreateInfo uniformBufferInfo = {};
	uniformBufferInfo.size = sizeof(Uniforms);
	uniformBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	uniformBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	VkResult res = vkCreateBuffer(device, &uniformBufferInfo, nullptr, &uniformBuffer);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements memoryReqs;
	vkGetBufferMemoryRequirements(device, uniformBuffer, &memoryReqs);

	VkMemoryAllocateInfo uniformMemoryInfo = {};
	uniformMemoryInfo.allocationSize = memoryReqs.size;
	uniformMemoryInfo.memoryTypeIndex = 
		getMemoryType(memoryReqs.memoryTypeBits,
								VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	uniformMemoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	res = vkAllocateMemory(device, &uniformMemoryInfo, nullptr, &uniformMemory);
	assert(res == VK_SUCCESS);

	res = vkBindBufferMemory(device, uniformBuffer, uniformMemory, 0);
	assert(res == VK_SUCCESS);

	uniformDescriptor.buffer = uniformBuffer;
	uniformDescriptor.offset = 0;
	uniformDescriptor.range = uniformBufferInfo.size;

	loadUniforms();
	createDescriptorPool();
	setupDescriptorSets();
}

void Vulkan::loadUniforms()
{
	static float y = 0.0f;
	uniforms.modelMatrix = glm::rotate(glm::mat4x4(), y, glm::vec3(0, 1, 1));
	uniforms.projectionMatrix = glm::perspective(glm::radians(70.0f), (float)surfaceExtent.width/ (float)surfaceExtent.height, 0.1f, 100.0f);
	uniforms.viewMatrix = glm::translate(glm::mat4x4(), glm::vec3(0.0f, 0.0f, -5.0f));

	void* data;
	VkResult res = vkMapMemory(device, uniformMemory, 0, sizeof(uniforms), 0, &data);
	assert(res == VK_SUCCESS);
	memcpy(data, &uniforms, sizeof(uniforms));
	vkUnmapMemory(device, uniformMemory);

	y += 0.001f;
}

void Vulkan::loadTexture(const std::string & filename)
{
	// CREATE TEXTURE'S IMAGE


	int texWidth;
	int texHeight;
	int texComp;

	stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texComp, STBI_rgb_alpha);
	assert(pixels);


	VkImage hostImage;
	VkDeviceMemory imageMemory;

	vk::createImage(
		physicalDevice,
		device,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		hostImage, texWidth, texHeight, imageMemory);


	void* data;
	VkDeviceSize size = texHeight * texHeight * 4;
	VkResult res = vkMapMemory(device, imageMemory, 0, size, 0, &data);
	assert(res == VK_SUCCESS);
	memcpy(data, pixels, size_t(size));
	vkUnmapMemory(device, hostImage);

	// LAYOUTS TRANSITION
	VkImage deviceImage;
	VkDeviceMemory deviceMemory;

	
	vk::createImage(
		physicalDevice,
		device,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		deviceImage, texWidth, texHeight, deviceMemory);

	vk::changeImageLayout(graphicsQueue, hostImage, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, commandPool, device);
	vk::changeImageLayout(graphicsQueue, deviceImage, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandPool, device);

	vk::copyImage(graphicsQueue, hostImage, deviceImage, texWidth, texHeight, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, commandPool, device);
	vk::changeImageLayout(graphicsQueue, deviceImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandPool, device);
	
	texture.width = texWidth;
	texture.height = texHeight;
	texture.memory = deviceMemory;
	texture.image = deviceImage;
}

void Vulkan::loadSampler()
{
	// ACCESS THE IMAGE THROUGH THE VIEW
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.image = texture.image;

	VkResult res = vkCreateImageView(device, &viewInfo, nullptr, &texture.view);
	assert(res == VK_SUCCESS);

	// CREATING THE SAMPLER
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.maxAnisotropy = 8;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	res = vkCreateSampler(device, &samplerInfo, nullptr, &texture.sampler);
	assert(res == VK_SUCCESS);
}

uint32_t Vulkan::getMemoryType(uint32_t typeBits, VkFlags properties)
{
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

	for (uint32_t i = 0; i < 32; i++) {
		if ((typeBits & 1) == 1
			&& (deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
		typeBits >>= 1;
	}

	return -1;
}

void Vulkan::createDescriptorPool()
{
	VkDescriptorPoolSize uniformDescriptor = {};
	uniformDescriptor.descriptorCount = 1;
	uniformDescriptor.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	VkDescriptorPoolSize samplerDescriptor = {};
	samplerDescriptor.descriptorCount = 1;
	samplerDescriptor.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolSize descriptors[] = { uniformDescriptor, samplerDescriptor };
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.maxSets = 1;
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = descriptors;
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

	VkResult res = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);
	assert(res == VK_SUCCESS);
}

void Vulkan::setupDescriptorSets()
{
	VkDescriptorSetLayoutBinding uniformBinding = {};
	uniformBinding.descriptorCount = 1;
	uniformBinding.binding = 0;
	uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerBinding = {};
	samplerBinding.descriptorCount = 1;
	samplerBinding.binding = 1;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // The texture is not needed per vertex

	VkDescriptorSetLayoutBinding bindings[] = { uniformBinding, samplerBinding };
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
	descriptorSetLayoutInfo.bindingCount = 2;
	descriptorSetLayoutInfo.pBindings = bindings;
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	VkResult res = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout);
	assert(res == VK_SUCCESS);

	// Add a new descriptor using the descriptor pool
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
	descriptorSetAllocInfo.descriptorPool = descriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = 1;
	descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	res = vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet);
	assert(res == VK_SUCCESS);

	VkDescriptorImageInfo textureDescriptor = {};
	textureDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureDescriptor.imageView = texture.view;
	textureDescriptor.sampler = texture.sampler;

	// Match binding points to the descirptor set

	// Binding : 0
	VkWriteDescriptorSet writeDescriptorSets[2];
	writeDescriptorSets[0] = {};
	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSets[0].dstSet = descriptorSet;
	writeDescriptorSets[0].pBufferInfo = &uniformDescriptor;
	writeDescriptorSets[0].dstBinding = 0;

	// Binding : 1
	writeDescriptorSets[1] = {};
	writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[1].descriptorCount = 1;
	writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[1].dstSet = descriptorSet;
	writeDescriptorSets[1].pImageInfo = &textureDescriptor;
	writeDescriptorSets[1].dstBinding = 1;

	vkUpdateDescriptorSets(device, 2, writeDescriptorSets, 0, nullptr);
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

	for (int i = 0; i < swapchainImages.size(); i++) {
		graphicsCommandBuffers.push_back(vk::createCommandBuffer(commandPool, device));
	}
}

void Vulkan::recordDrawCommand()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1;

	VkClearValue clearValues[2];
	clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0.0f;

	// PIPELINE BARRIER NOT NEEDED IF THERE IS A RENDERPASS

	VkRenderPassBeginInfo renderPassBegin = {};
	renderPassBegin.clearValueCount = 2;
	renderPassBegin.pClearValues = clearValues;
	renderPassBegin.renderArea.extent = surfaceExtent;
	renderPassBegin.renderArea.offset = { 0, 0 };
	renderPassBegin.renderPass = renderPass;
	renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	VkDeviceSize offsets = { 0 };

	for (uint32_t i = 0; i < swapchainImages.size(); i++) {
		renderPassBegin.framebuffer = frameBuffers[i];

		vkBeginCommandBuffer(graphicsCommandBuffers[i], &beginInfo);
		vkCmdBeginRenderPass(graphicsCommandBuffers[i], &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
		
		vkCmdBindDescriptorSets(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		vkCmdBindPipeline(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		vkCmdBindVertexBuffers(graphicsCommandBuffers[i], VERTEX_BINDING_ID, 1, &vertexBuffer, &offsets);
		vkCmdBindIndexBuffer(graphicsCommandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(graphicsCommandBuffers[i], 6, 1, 0, 0, 1);

		vkCmdEndRenderPass(graphicsCommandBuffers[i]);
		vkEndCommandBuffer(graphicsCommandBuffers[i]);
	}
}

void Vulkan::createRenderPass()
{
	VkAttachmentDescription attachmentDescriptions[2];

	attachmentDescriptions[0] = {};
	attachmentDescriptions[0].format = surfaceFormat.format;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	
	attachmentDescriptions[1] = {};
	attachmentDescriptions[1].format = depthFormat;
	attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	VkAttachmentReference colorAttachment = {};
	colorAttachment.attachment = 0;
	colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachment = {};
	depthAttachment.attachment = 1;
	depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	VkSubpassDescription subPassDescription = {};
	subPassDescription.colorAttachmentCount = 1;
	subPassDescription.pColorAttachments = &colorAttachment;
	subPassDescription.pDepthStencilAttachment = &depthAttachment;
	subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachmentDescriptions;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subPassDescription;
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	VkResult res = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
	assert(res == VK_SUCCESS);
}

void Vulkan::createGraphicsPipeline()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = 0xF;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachment;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.blendConstants[0] = 0.0f;
	colorBlendState.blendConstants[1] = 0.0f;
	colorBlendState.blendConstants[2] = 0.0f;
	colorBlendState.blendConstants[3] = 0.0f;
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.minSampleShading = 1.0f;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.lineWidth = 1.0f;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;


	VkVertexInputBindingDescription vertexBindingDescription = {};
	vertexBindingDescription.binding = VERTEX_BINDING_ID;
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexBindingDescription.stride = sizeof(Vertex);

	VkVertexInputAttributeDescription positionDescription = {};
	positionDescription.binding = VERTEX_BINDING_ID;
	positionDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	positionDescription.location = 0;
	positionDescription.offset = offsetof(Vertex, position);

	VkVertexInputAttributeDescription colorDescription = {};
	colorDescription.binding = VERTEX_BINDING_ID;
	colorDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	colorDescription.location = 1;
	colorDescription.offset = offsetof(Vertex, color);

	VkVertexInputAttributeDescription uvDescription = {};
	uvDescription.binding = VERTEX_BINDING_ID;
	uvDescription.format = VK_FORMAT_R32G32_SFLOAT;
	uvDescription.location = 2;
	uvDescription.offset = offsetof(Vertex, uv);

	VkVertexInputAttributeDescription attributeDescriptions[] = {
		positionDescription, colorDescription, uvDescription
	};

	VkPipelineVertexInputStateCreateInfo vertexInput = {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInput.vertexAttributeDescriptionCount = 3; // COLOR, POSITION, UV
	vertexInput.pVertexAttributeDescriptions = attributeDescriptions;

	VkViewport viewport;
	viewport.height = surfaceExtent.height;
	viewport.width = surfaceExtent.width;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.x = 0.0f;
	viewport.y = 0.0f;

	VkRect2D scissor;
	scissor.extent = surfaceExtent;
	scissor.offset.x = 0;
	scissor.offset.y = 0;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

	VkPipelineShaderStageCreateInfo shadersStages[] = {
		createShaderStage("shaders/colorVert.spirv", VK_SHADER_STAGE_VERTEX_BIT),	// VERTEX SHADER
		createShaderStage("shaders/colorFrag.spirv", VK_SHADER_STAGE_FRAGMENT_BIT),	// FRAGMENT SHADER
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilState.front = depthStencilState.back;
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {};
	graphicsPipelineInfo.basePipelineIndex = -1;
	graphicsPipelineInfo.subpass = 0;
	graphicsPipelineInfo.renderPass = renderPass;
	graphicsPipelineInfo.layout = pipelineLayout;
	graphicsPipelineInfo.pColorBlendState = &colorBlendState;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssembly;
	graphicsPipelineInfo.pMultisampleState = &multisampleState;
	graphicsPipelineInfo.pRasterizationState = &rasterizationState;
	graphicsPipelineInfo.pVertexInputState = &vertexInput;
	graphicsPipelineInfo.pViewportState = &viewportState;
	graphicsPipelineInfo.pDepthStencilState = &depthStencilState;
	graphicsPipelineInfo.stageCount = 2;
	graphicsPipelineInfo.pStages = shadersStages;
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	VkResult res = vkCreateGraphicsPipelines(device, 0, 1, &graphicsPipelineInfo, nullptr, &graphicsPipeline);
	assert(res == VK_SUCCESS);

	vkDestroyShaderModule(device, vertexShaderModule, nullptr);
	vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
}

VkPipelineShaderStageCreateInfo Vulkan::createShaderStage(const std::string& filename, VkShaderStageFlagBits shaderStage)
{
	std::ifstream file(filename, std::ios::binary);
	file.seekg(0, std::ios::end);
	std::vector<char> code(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read(code.data(), code.size());

	VkShaderModuleCreateInfo shaderModuleInfo = {};
	shaderModuleInfo.codeSize = code.size();
	shaderModuleInfo.pCode = (uint32_t*) code.data();
	shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	VkShaderModule shaderModule;
	if (shaderStage == VK_SHADER_STAGE_VERTEX_BIT) {
		vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &vertexShaderModule);
		shaderModule = vertexShaderModule;
	}
	else {
		vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &fragmentShaderModule);
		shaderModule = fragmentShaderModule;
	}

	VkPipelineShaderStageCreateInfo shaderStageInfo = {};
	shaderStageInfo.module = shaderModule;
	shaderStageInfo.pName = "main";
	shaderStageInfo.stage = shaderStage;
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	return shaderStageInfo;
}

void Vulkan::createFrameBuffers()
{
	frameBuffers.resize(swapchainImages.size());

	VkImageView attachments[2];
	attachments[1] = depthBufferImageView;

	VkFramebufferCreateInfo frameBufferInfo = {};
	frameBufferInfo.height = surfaceExtent.height;
	frameBufferInfo.width = surfaceExtent.width;
	frameBufferInfo.layers = 1;
	frameBufferInfo.attachmentCount = 2;
	frameBufferInfo.renderPass = renderPass;
	frameBufferInfo.pAttachments = attachments;
	frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

	VkResult res;
	for (int i = 0; i < swapchainImages.size(); i++) {
		attachments[0] = swapchainImageViews[i];
		res = vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &frameBuffers[i]);
		assert(res == VK_SUCCESS);
	}
}

Vulkan::~Vulkan()
{
	vkDeviceWaitIdle(device);
	vkQueueWaitIdle(graphicsQueue);
	
	vkFreeMemory(device, indexMemory, nullptr);
	vkFreeMemory(device, vertexMemory, nullptr);
	vkFreeMemory(device, uniformMemory, nullptr);
	vkFreeMemory(device, depthBufferMemory, nullptr);

	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkDestroyBuffer(device, uniformBuffer, nullptr);

	vkDestroyImageView(device, depthBufferImageView, nullptr);
	vkDestroyImage(device, depthBufferImage, nullptr);

	for (auto& frameBuffer : frameBuffers) {
		vkDestroyFramebuffer(device, frameBuffer, nullptr);
	}
	for (auto& imageView : swapchainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroySemaphore(device, imageIsAvailable, nullptr);
	vkDestroySemaphore(device, imageIsRendered, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
}
