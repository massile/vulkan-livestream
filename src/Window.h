#pragma once

#include <string>
#include <GLFW\glfw3.h>

class Window
{
private:
	int width;
	int height;
	std::string title;

	GLFWwindow *window;

public:
	Window(const std::string& title, int width, int height);
	void clear();

	bool shouldClose();

	inline int getWidth() const { return width; }
	inline int getHeight() const { return height; }
	inline std::string getTitle() const { return title; }
};