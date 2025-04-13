#include "vulkan/init.h"
#include "vulkan/setup.h"
#include "vulkan/run.h"

#include "vulkan/global.h"

int main(void)
{
	initVulkan();

	setupVulkan();

	while(processMainInput())
	{
		drawFrame();
	}

	cleanupSetupVulkan();

	cleanupInitVulkan();

	return 0;
}

bool processInput(SDL_Event event)
{
	return true;
}
