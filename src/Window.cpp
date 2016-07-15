#include "Window.h"
#include <Windows.h>
#include "Vulkan.h"

Window::Window(const std::string & title, int width, int height)
{
	glfwInit();

	this->title = title;
	this->width = width;
	this->height = height;
	window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	Vulkan::app.createSurface(window);
	Vulkan::app.init();
}

bool Window::shouldClose()
{
	return glfwWindowShouldClose(window) == 1;
}

void Window::clear()
{
	glfwPollEvents();
}