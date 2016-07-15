#include "Vulkan.h"
#include <iostream>
#include <assert.h>
#include <fstream>

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
	createRenderPass();
	createFrameBuffers();
	createGraphicsPipeline();
	createCommandBuffers();
	recordDrawCommand();
}

void Vulkan::draw()
{
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

	VkClearValue clearColor = {
		{0.0f, 0.0f, 0.0f, 1.0f}
	};

	// PIPELINE BARRIER NOT NEEDED IF THERE IS A RENDERPASS

	VkRenderPassBeginInfo renderPassBegin = {};
	renderPassBegin.clearValueCount = 1;
	renderPassBegin.pClearValues = &clearColor;
	renderPassBegin.renderArea.extent = surfaceExtent;
	renderPassBegin.renderArea.offset = { 0, 0 };
	renderPassBegin.renderPass = renderPass;
	renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	for (uint32_t i = 0; i < swapchainImages.size(); i++) {
		renderPassBegin.framebuffer = frameBuffers[i];

		vkBeginCommandBuffer(graphicsCommandBuffers[i], &beginInfo);
		vkCmdBeginRenderPass(graphicsCommandBuffers[i], &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
		
		vkCmdBindPipeline(graphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		vkCmdDraw(graphicsCommandBuffers[i], 3, 1, 0, 0); // The vertices are defined in the shader

		vkCmdEndRenderPass(graphicsCommandBuffers[i]);
		vkEndCommandBuffer(graphicsCommandBuffers[i]);
	}
}

void Vulkan::createRenderPass()
{
	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.format = surfaceFormat.format;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	
	VkAttachmentReference colorAttachment = {};
	colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPassDescription = {};
	subPassDescription.colorAttachmentCount = 1;
	subPassDescription.pColorAttachments = &colorAttachment;
	subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachmentDescription;
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

	VkPipelineLayout pipelineLayout;
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
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.lineWidth = 1.0f;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	// TODO: Add vertex attributes, vertices will be defined in shader for now
	VkPipelineVertexInputStateCreateInfo vertexInput = {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

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
	graphicsPipelineInfo.stageCount = 2;
	graphicsPipelineInfo.pStages = shadersStages;
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	VkResult res = vkCreateGraphicsPipelines(device, 0, 1, &graphicsPipelineInfo, nullptr, &graphicsPipeline);
	assert(res == VK_SUCCESS);
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
	vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule);

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

	VkFramebufferCreateInfo frameBufferInfo = {};
	frameBufferInfo.height = surfaceExtent.height;
	frameBufferInfo.width = surfaceExtent.width;
	frameBufferInfo.layers = 1;
	frameBufferInfo.attachmentCount = 1;
	frameBufferInfo.renderPass = renderPass;
	frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

	VkResult res;
	for (int i = 0; i < swapchainImages.size(); i++) {
		frameBufferInfo.pAttachments = &swapchainImageViews[i];
		res = vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &frameBuffers[i]);
		assert(res == VK_SUCCESS);
	}
}

Vulkan::~Vulkan()
{
	vkDeviceWaitIdle(device);
	for (auto& frameBuffer : frameBuffers) {
		vkDestroyFramebuffer(device, frameBuffer, nullptr);
	}
	for (auto& imageView : swapchainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroySemaphore(device, imageIsAvailable, nullptr);
	vkDestroySemaphore(device, imageIsRendered, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
}
