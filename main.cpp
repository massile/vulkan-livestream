#include "src/Window.h"

int main()
{
	Window window("Vulkan", 800, 600);
	
	while (!window.shouldClose()) {
		window.clear();
	}

	return 0;
}