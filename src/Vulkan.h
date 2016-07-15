#pragma once

#include <vulkan\vulkan.h>
#include <vector>

class Vulkan
{
private:
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	uint32_t graphicsFamilyIndex = -1;

public:
	static Vulkan app;

	Vulkan();
	virtual ~Vulkan();

private:
	void createInstance();
	void createDevice();
	uint32_t chooseQueueFamilyIndex();
};
