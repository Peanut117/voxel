#include <limits.h>

#include "vkinitialize.h"

#include "pisdef.h"
#include "engine.h"
#include "swapchain.h"
#include "validationlayers.h"

#include "vulkan/vulkan_beta.h"
#include "vulkan/vulkan_core.h"

/* =====================Add needed instance extentions here========================= */

#ifdef DEBUG
const uint32_t instanceExtentionCount = 2;

const char* instanceExtentions[] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
};
#else
const uint32_t instanceExtentionCount = 1;

const char* instanceExtentions[] = {
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
};
#endif

/* ======================Add needed device extentions here=========================== */

const uint32_t deviceExtentionCount = 2;
const char* deviceExtentions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
};

/* =================================Helper functions================================ */

const char** GetExtentionNames(uint32_t* extentionCount);
void SelectPhysicalDevice(VkInstance instance, VkPhysicalDevice* pDevice);
QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice pDevice);

/* ================================================================================ */

void InitVulkan(PisEngine* pis)
{
    VK_CHECK(volkInitialize());

    VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "voxel engine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4
    };

    uint32_t extentionCount = 0;
    const char** extentionNames = GetExtentionNames(&extentionCount);

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR, //Needed for Mac, has something to do with metal
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = extentionCount,
        .ppEnabledExtensionNames = extentionNames
    };

#ifdef DEBUG
    if(!CheckValidationLayerSupport())
    {
        fprintf(stderr, "Validation layers requested but not available\n");
    }

    instanceCreateInfo.enabledLayerCount = GetValidationLayerCount();
    instanceCreateInfo.ppEnabledLayerNames = GetValidationLayers();
#endif

    //TODO: Build memory allocator
    VK_CHECK(vkCreateInstance(&instanceCreateInfo, NULL, &pis->vk.instance));

    free(extentionNames);

    // Volk will load all required vulkan entrypoints, including all extentions
    volkLoadInstance(pis->vk.instance);

#ifdef DEBUG
    SetupDebugMessenger(pis->vk.instance, &pis->vk.debugMessenger, false);
#endif

    // Create the vulkan rendering surface with SDL
    SDL_Vulkan_CreateSurface(pis->window, pis->vk.instance, NULL, &pis->vk.surface);

    // Pick physical device
    SelectPhysicalDevice(pis->vk.instance, &pis->vk.physicalDevice);

    // Find available queues
    pis->vk.indices = FindQueueFamilies(pis->vk.physicalDevice);

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = pis->vk.indices.computeFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    VkPhysicalDeviceFeatures2 features = {0};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = NULL;
    
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features,
        .pQueueCreateInfos = &queueCreateInfo,
        .queueCreateInfoCount = 1,
		.enabledExtensionCount = deviceExtentionCount,
		.ppEnabledExtensionNames = deviceExtentions,
        .pEnabledFeatures = NULL,
    };

    VK_CHECK(vkCreateDevice(pis->vk.physicalDevice, &deviceCreateInfo, NULL, &pis->vk.device));

    volkLoadDevice(pis->vk.device);

    vkGetDeviceQueue(pis->vk.device, pis->vk.indices.computeFamilyIndex, 0, &pis->vk.computeQueue);

    CreateSwapchain(pis, pis->windowExtent.width, pis->windowExtent.height);
}


const char** GetExtentionNames(uint32_t* extentionCount)
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

    VkExtensionProperties properties[propertyCount];
    vkEnumerateInstanceExtensionProperties(NULL, &propertyCount, properties);

    for(uint32_t i = 0; i < instanceExtentionCount; i++)
    {
        bool extentionFound = false;

        for(uint32_t j = 0; j < propertyCount; j++)
        {
            if(strcmp(properties[j].extensionName, instanceExtentions[i]) == 0)
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

    //Add extra extentions to the start of the extentions array
    memcpy(extentions, instanceExtentions, instanceExtentionCount * sizeof(const char*));

    return extentions;
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice pDevice)
{
    QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, NULL);

	VkQueueFamilyProperties queueFamilyProperties[queueFamilyCount];
	vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, queueFamilyProperties);

	for(uint32_t i = 0; i < queueFamilyCount; i++)
	{
		// uint32_t presentSupported = false;
		// vkGetPhysicalDeviceSurfaceSupportKHR(pDevice, i, surface, &presentSupported);

		if(queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT/* && presentSupported*/)
		{
			indices.computeFamilyIndex = i;
			indices.computeFamilyIsAvailable = true;
			break;
		}
	}

	if(!indices.computeFamilyIsAvailable)
	{
		fprintf(stderr, "Not all requested queue families available\n");
        exit(-1);
	}

    return indices;
}

bool CheckDeviceExtentionSupport(VkPhysicalDevice device)
{
	//Easy check for device extentions
	if(deviceExtentionCount == 0)
		return true;

	uint32_t availableExtentionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, NULL, &availableExtentionCount, NULL);

	VkExtensionProperties availableExtentions[availableExtentionCount];
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

	return extentionFound;
}

int RateDevice(VkPhysicalDevice pDevice)
{
    int score = 0;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures2 features;
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    vkGetPhysicalDeviceProperties(pDevice, &properties);
    vkGetPhysicalDeviceFeatures2(pDevice, &features);

    QueueFamilyIndices indices = FindQueueFamilies(pDevice);
    if(!indices.computeFamilyIsAvailable)
    {
        fprintf(stderr, "Needed queue families not found\n");
        return -1;
    }

    if(!CheckDeviceExtentionSupport(pDevice))
    {
        fprintf(stderr, "Device extentions not found\n");
        return -1;
    }

    if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 100;
    } else if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        score += 50;
    }

    return score;
}

void SelectPhysicalDevice(VkInstance instance, VkPhysicalDevice* pDevice)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

	if(deviceCount == 0)
	{
		fprintf(stderr, "Failed to find GPU with vulkan support\n");
		exit(-1);
	}

    // Gather all devices
	VkPhysicalDevice devices[deviceCount];
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    *pDevice = devices[0];

    // Loop over every device and give it a rating
	for(size_t i = 0; i < deviceCount; i++)
	{
        // Highest ratings gets to be the device
        if(RateDevice(*pDevice) < RateDevice(devices[i]))
        {
            *pDevice = devices[i];
        }
	}

    // If no device found
	if(*pDevice == VK_NULL_HANDLE)
	{
		fprintf(stderr, "Failed to find a suitable GPU\n");
		exit(-1);
	}
}
