#include "Vulkan.h"
#include <iostream>
#include <assert.h>

Vulkan Vulkan::app;

Vulkan::Vulkan()
{
	createInstance();
	createDevice();
}

Vulkan::~Vulkan()
{
	vkDestroyInstance(instance, nullptr);
}

void Vulkan::createInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pApplicationName = "Vulkan Application";
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

	// These extensions are required to display something on the screen
	const char* extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.enabledExtensionCount = 2;
	instanceInfo.ppEnabledExtensionNames = extensions;
	instanceInfo.pApplicationInfo = &appInfo;
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
