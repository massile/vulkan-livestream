#pragma once

#include <vulkan\vulkan.h>
#include <vector>

class Vulkan
{
private:
	VkInstance instance;

public:
	static Vulkan app;

	Vulkan();
	virtual ~Vulkan();

private:
	void createInstance();
};
