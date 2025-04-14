#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_beta.h>

#include <SDL3/SDL_vulkan.h>

#include "init.h"
#include "define.h"
#include "global.h"
#include "vulkanGlobal.h"

/***********Structs***********/

/***********Constants***********/
const char* WINDOW_TITLE = "Vulkan engine";

const uint32_t instanceExtentionCount = 1;
const char* instanceExtentions[] = {
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
};

const uint32_t deviceExtentionCount = 2;
const char* deviceExtentions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
};

/***********Vulkan Handles*************/
SDL_Window* window; //Global

VkInstance instance;
VkSurfaceKHR surface; //Global
VkPhysicalDevice physicalDevice; //Global
VkDevice device; //Global
VkQueue graphicsQueue; //Global

/************Variables**********/

/************Function Declarations**********/
int createWindow(void);
int createInstance(void);
int createSurface(void);
int pickPhysicalDevice(void);
int createDevice(void);

/***********Helper Function Declarations***********/
const char* const* getExtentionNames(uint32_t* extentionCount);
bool isDeviceSuitable(VkPhysicalDevice device);
bool checkDeviceExtentionSupport(VkPhysicalDevice device);
QueueFamilyIndices gatherQueueFamilies(VkPhysicalDevice device);

/***********Create Global Functions***********/

int initVulkan(void)
{
	createWindow();
	createInstance();
	createSurface();
	pickPhysicalDevice();
	createDevice();

	return 0;
}

void cleanupInitVulkan(void)
{
	vkDestroyDevice(device, NULL);

	SDL_Vulkan_DestroySurface(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);

	SDL_DestroyWindow(window);
	window = NULL;
}


/***********Create Functions***********/

int createWindow(void)
{
	window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	return 0;
}

int createInstance(void)
{
	VkApplicationInfo applicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = "vulkanEngine",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = NULL,
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_4
	};

	uint32_t extentionCount = 0;
	const char* const* extentionNames = getExtentionNames(&extentionCount);

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR, //Needed for Mac, has something to do with metal
		.pApplicationInfo = &applicationInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = extentionCount,
		.ppEnabledExtensionNames = extentionNames
	};

	//Build memory allocator
	VK_CHECK(vkCreateInstance(&createInfo, NULL, &instance));

	return 0;
}

int createSurface(void)
{
	if(!SDL_Vulkan_CreateSurface(window, instance, NULL, &surface))
	{
		fprintf(stderr, "Failed to create surface\n");
		return -1;
	}

	return 0;
}

int pickPhysicalDevice(void)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

	if(deviceCount == 0)
	{
		fprintf(stderr, "Failed to find GPU with vulkan support\n");
		return -1;
	}

	VkPhysicalDevice* devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

	for(size_t i = 0; i < deviceCount; i++)
	{
		if(isDeviceSuitable(devices[i]))
		{
			physicalDevice = devices[i];
			break;
		}
	}

	if(physicalDevice == VK_NULL_HANDLE)
	{
		fprintf(stderr, "Failed to find a suitable GPU\n");
		return -1;
	}

	free(devices);

	return 0;
}

int createDevice(void)
{
	QueueFamilyIndices indices = gatherQueueFamilies(physicalDevice);

	float queuePriorities[] = {1.0f};

	VkDeviceQueueCreateInfo queueCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = indices.graphicsFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = queuePriorities
	};

	VkDeviceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = deviceExtentionCount,
		.ppEnabledExtensionNames = deviceExtentions,
		.pEnabledFeatures = NULL
	};

	VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, NULL, &device));

	vkGetDeviceQueue(device, indices.graphicsFamilyIndex, 0, &graphicsQueue);

	return 0;
}

const char* const* getExtentionNames(uint32_t* extentionCount)
{
	//Asks the sdl api for all the instance extentions sdl needs
	uint32_t sdlExtensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
	*extentionCount = sdlExtensionCount + instanceExtentionCount;

	char const* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(NULL);

	const char** extentions = malloc(*extentionCount * sizeof(const char*));

	//Add sdl extentions to the extentions array
	//Keep room for the additonal extentions of needed at the start of the array
	memcpy(&extentions[instanceExtentionCount], sdlExtensions, (*extentionCount - instanceExtentionCount) * sizeof(const char*));

	//If we ask for no additional extentions, return the sdl extentions
	if(instanceExtentionCount == 0)
		return extentions;

	//If we do need additional extentions, check if these are available
	
	uint32_t propertyCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &propertyCount, NULL);

	VkExtensionProperties* properties = malloc(propertyCount * sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(NULL, &propertyCount, properties);

	for(uint32_t i = 0; i < instanceExtentionCount; i++)
	{
		bool extentionFound = false;

		for(uint32_t j = 0; j < propertyCount; j++)
		{
			if(strcmp(properties[j].extensionName, instanceExtentions[i]))
			{
				extentionFound = true;
				break;
			}
		}

		if(!extentionFound)
		{
			fprintf(stderr, "Instance extention not avaialable: %s\n", instanceExtentions[i]);
			return extentions;
		}
	}

	free(properties);

	//Add extra extentions to the start of the extentions array
	memcpy(extentions, instanceExtentions, instanceExtentionCount * sizeof(const char*));

	return extentions;
}

bool isDeviceSuitable(VkPhysicalDevice device)
{
	//Asks for all the queue families to check if the needed queue families are there
	QueueFamilyIndices indices = gatherQueueFamilies(device);

	//Checks if this device support all the needed device extentions
	bool extentionsSupported = checkDeviceExtentionSupport(device);

	swapchainSupportDetails swapchainDetails = gatherSwapchainSupportDetails(device);
	if(swapchainDetails.surfaceFormatCount == 0)
		return false;

	return extentionsSupported && indices.graphicsFamilyIsSet;
}

bool checkDeviceExtentionSupport(VkPhysicalDevice device)
{
	//Easy check for device extentions
	
	if(deviceExtentionCount == 0)
		return true;

	uint32_t availableExtentionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, NULL, &availableExtentionCount, NULL);

	VkExtensionProperties* availableExtentions = malloc(availableExtentionCount * sizeof(VkExtensionProperties));
	vkEnumerateDeviceExtensionProperties(device, NULL, &availableExtentionCount, availableExtentions);

	bool extentionFound = false;

	for(uint32_t i = 0; i < deviceExtentionCount; i++)
	{
		for(uint32_t j = 0; j < availableExtentionCount; j++)
		{
			if(strcmp(deviceExtentions[i], availableExtentions[j].extensionName) == 0)
			{
				extentionFound = true;
				break;
			}
		}
	}

	free(availableExtentions);

	return extentionFound;
}

QueueFamilyIndices gatherQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = {0};

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

	VkQueueFamilyProperties* queueFamilyProperties = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties);

	for(uint32_t i = 0; i < queueFamilyCount; i++)
	{
		uint32_t presentSupported = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupported);

		if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupported)
		{
			indices.graphicsFamilyIndex = i;
			indices.graphicsFamilyIsSet = true;
			break;
		}
	}

	free(queueFamilyProperties);

	if(!indices.graphicsFamilyIsSet)
	{
		fprintf(stderr, "Not all requested queue families available\n");
	}

	return indices;
}
