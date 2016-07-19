#pragma once

namespace vk {

	uint32_t getMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeBits, VkFlags properties)
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

	void flushCommandBuffer(VkCommandBuffer& cmdBuffer, VkQueue graphicsQueue)
	{
		vkEndCommandBuffer(cmdBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer; // We want to send on the ith swapchain
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// Sends the draw command to the GPU (draws in the buffers)
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, 0);
	}

	VkCommandBuffer createCommandBuffer(VkCommandPool& commandPool, VkDevice& device)
	{
		VkCommandBufferAllocateInfo bufferAllocInfo = {};
		bufferAllocInfo.commandBufferCount = 1;
		bufferAllocInfo.commandPool = commandPool;
		bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

		VkCommandBuffer cmdBuffer;
		VkResult res = vkAllocateCommandBuffers(device, &bufferAllocInfo, &cmdBuffer);
		assert(res == VK_SUCCESS);

		return cmdBuffer;
	}
	
	VkCommandBuffer createAndBeginCommandBuffer(VkFlags flags, VkCommandPool& commandPool, VkDevice& device)
	{
		VkCommandBuffer cmdBuffer = createCommandBuffer(commandPool, device);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.flags = flags;
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		vkBeginCommandBuffer(cmdBuffer, &beginInfo);

		return cmdBuffer;
	}

	void copyImage(VkQueue& queue, VkImage srcImage, VkImage dstImage, int width, int height, VkFlags flags, VkCommandPool& commandPool, VkDevice& device)
	{
		VkCommandBuffer cmdBuffer = createAndBeginCommandBuffer(flags, commandPool, device);

		VkImageSubresourceLayers subResources = {};
		subResources.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subResources.baseArrayLayer = 0;
		subResources.layerCount = 1;
		subResources.mipLevel = 0;

		VkImageCopy copy;
		copy.dstOffset.x = 0;
		copy.dstOffset.y = 0;
		copy.dstOffset.z = 0;
		copy.srcOffset.x = 0;
		copy.srcOffset.y = 0;
		copy.srcOffset.z = 0;
		copy.srcSubresource = subResources;
		copy.dstSubresource = subResources;
		copy.extent.width = width;
		copy.extent.height = height;
		copy.extent.depth = 1;

		vkCmdCopyImage(
			cmdBuffer,
			srcImage,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &copy
		);

		flushCommandBuffer(cmdBuffer, queue);
		vkQueueWaitIdle(queue);
		vkFreeCommandBuffers(device, commandPool, 1, &cmdBuffer);
	}

	void changeImageLayout(VkQueue& queue, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool& commandPool, VkDevice& device)
	{
		VkCommandBuffer cmdBuffer = createAndBeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
																commandPool, device);

		VkImageMemoryBarrier memoryBarrier = {};
		memoryBarrier.image = image;
		memoryBarrier.newLayout = newLayout;
		memoryBarrier.oldLayout = oldLayout;
		memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		memoryBarrier.subresourceRange.baseArrayLayer = 0;
		memoryBarrier.subresourceRange.baseMipLevel = 0;
		memoryBarrier.subresourceRange.layerCount = 1;
		memoryBarrier.subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
							 0, 0, 0, 0, 0, 1, &memoryBarrier);
		
		switch (oldLayout)
		{
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		}


		switch (newLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		}
		
		flushCommandBuffer(cmdBuffer, queue);
		vkQueueWaitIdle(queue);
		vkFreeCommandBuffers(device, commandPool, 1, &cmdBuffer);
	}

	void createImage(VkPhysicalDevice& physicalDevice, VkDevice& device, VkFlags props, VkImageTiling tiling, VkImageUsageFlags usage, VkImage& image, int w, int h, VkDeviceMemory& memory)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.arrayLayers = 1;
		imageInfo.extent.width = w;
		imageInfo.extent.height = h;
		imageInfo.extent.depth = 1;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		imageInfo.mipLevels = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.tiling = tiling; //TODO: STAGING LATER
		imageInfo.usage = usage;
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

		VkResult res = vkCreateImage(device, &imageInfo, nullptr, &image);
		assert(res == VK_SUCCESS);

		// ALLOCATES MEMORY FOR IT

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device, image, &memReqs);

		VkMemoryAllocateInfo memoryAllocInfo = {};
		memoryAllocInfo.allocationSize = memReqs.size;
		memoryAllocInfo.memoryTypeIndex = getMemoryType(physicalDevice, memReqs.memoryTypeBits, props);
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		res = vkAllocateMemory(device, &memoryAllocInfo, nullptr, &memory);
		assert(res == VK_SUCCESS);

		res = vkBindImageMemory(device, image, memory, 0);
		assert(res == VK_SUCCESS);
	}
}