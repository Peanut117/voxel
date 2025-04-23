#include "swapchain.h"
#include "initializers.h"
#include "pisdef.h"
#include <limits.h>

void CreateSwapchain(PisEngine* pis, uint32_t width, uint32_t height)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pis->vk.physicalDevice, pis->vk.surface, &surfaceCapabilities);

    uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(pis->vk.physicalDevice, pis->vk.surface, &presentModeCount, NULL);

    VkPresentModeKHR presentModes[presentModeCount];
	vkGetPhysicalDeviceSurfacePresentModesKHR(pis->vk.physicalDevice, pis->vk.surface, &presentModeCount, presentModes);

    uint32_t surfaceFormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(pis->vk.physicalDevice, pis->vk.surface, &surfaceFormatCount, NULL);

	VkSurfaceFormatKHR surfaceFormats[surfaceFormatCount];
	vkGetPhysicalDeviceSurfaceFormatsKHR(pis->vk.physicalDevice, pis->vk.surface, &surfaceFormatCount, surfaceFormats);

/* ===================================Choose the swap surface format==================================== */
    uint32_t surfaceFormatIndex = 0;
	for(uint32_t i = 0; i < surfaceFormatCount; i++)
	{
		if(surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			surfaceFormatIndex = i;
		}
	}

    pis->vk.swapchainImageFormat = surfaceFormats[surfaceFormatIndex].format;
    pis->vk.swapchainColorSpace = surfaceFormats[surfaceFormatIndex].colorSpace;

/* ===================================Choose the swap extent==================================== */
	VkExtent2D extent = {
		.width = (uint32_t)width,
		.height = (uint32_t)height
	};

    // If window is minimalized
	if(surfaceCapabilities.currentExtent.width != UINT_MAX)
		pis->vk.swapchainExtent = surfaceCapabilities.currentExtent;

	//Clamp width and height between the allowed extents that are supported
	pis->vk.swapchainExtent.width = extent.width < surfaceCapabilities.minImageExtent.width ? surfaceCapabilities.minImageExtent.width : extent.width;
	pis->vk.swapchainExtent.width = extent.width > surfaceCapabilities.maxImageExtent.width ? surfaceCapabilities.maxImageExtent.width : extent.width;

	pis->vk.swapchainExtent.height = extent.height < surfaceCapabilities.minImageExtent.height ? surfaceCapabilities.minImageExtent.height : extent.height;
	pis->vk.swapchainExtent.height = extent.height > surfaceCapabilities.maxImageExtent.height ? surfaceCapabilities.maxImageExtent.height : extent.height;

/* ===================================Choose the swap present mode==================================== */
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for(uint32_t i = 0; i < presentModeCount; i++)
	{
		if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			presentMode = presentModes[i];
	}

/* ===================================Swapchain image count==================================== */
	uint32_t imageCount = (!surfaceCapabilities.maxImageCount && surfaceCapabilities.minImageCount+1 > surfaceCapabilities.maxImageCount) ?
				surfaceCapabilities.minImageCount + 1 :
				surfaceCapabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.surface = pis->vk.surface,
		.minImageCount = imageCount,
		.imageFormat = pis->vk.swapchainImageFormat,
		.imageColorSpace = pis->vk.swapchainColorSpace,
		.imageExtent = pis->vk.swapchainExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.preTransform = surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

    VK_CHECK(vkCreateSwapchainKHR(pis->vk.device, &createInfo, NULL, &pis->vk.swapchain));

	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(pis->vk.device, pis->vk.swapchain, &swapchainImageCount, NULL);
	pis->vk.swapchainImages = malloc(swapchainImageCount * sizeof(VkImage));
	vkGetSwapchainImagesKHR(pis->vk.device, pis->vk.swapchain, &swapchainImageCount, pis->vk.swapchainImages);

    pis->vk.swapchainImageViews = malloc(swapchainImageCount * sizeof(VkImageView));

    for(uint32_t i = 0; i < swapchainImageCount; i++)
    {
        VkImageViewCreateInfo imageViewInfo = ImageViewCreateInfo(pis->vk.swapchainImageFormat,
                                                                  pis->vk.swapchainImages[i],
                                                                  VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(pis->vk.device, &imageViewInfo, NULL, &pis->vk.swapchainImageViews[i]));
    }

    pis->vk.swapchainImageCount = swapchainImageCount;
}

void CreateDrawImages(PisEngine* pis, uint32_t width, uint32_t height)
{
    // Create images to draw to
    VkExtent3D drawImageExtent = {
        width,
        height,
        1
    };

    pis->vk.drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    pis->vk.drawImage.extent = drawImageExtent;

    VkImageUsageFlags drawImageUsages = {0};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    CreateImage(pis->vk.device, pis->vk.physicalDevice,
                pis->vk.drawImage.format,
                drawImageUsages, pis->vk.drawImage.extent,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &pis->vk.drawImage.image,
                &pis->vk.drawImage.memory);

    VkImageViewCreateInfo imgViewInfo = ImageViewCreateInfo(pis->vk.drawImage.format, pis->vk.drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(pis->vk.device, &imgViewInfo, NULL, &pis->vk.drawImage.view));
}
