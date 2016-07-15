#include "Window.h"
#include <Windows.h>

Window::Window(const std::string & title, int width, int height)
{
	glfwInit();

	this->title = title;
	this->width = width;
	this->height = height;
	window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

bool Window::shouldClose()
{
	return glfwWindowShouldClose(window) == 1;
}

void Window::clear()
{
	glfwPollEvents();
}