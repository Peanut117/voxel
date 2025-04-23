#ifndef IMAGES_H
#define IMAGES_H

#include "volk.h"
#include "vulkan/vulkan_core.h"

typedef struct AllocatedImage {
    VkImage image;
    VkImageView view;
    VkExtent3D extent;
    VkFormat format;
    VkDeviceMemory memory;
} AllocatedImage;

void CreateImage(VkDevice device,
                 VkPhysicalDevice pDevice,
                 VkFormat format,
                 VkImageUsageFlags imageUsage,
                 VkExtent3D extent,
                 VkMemoryPropertyFlags properties,
                 VkImage* image,
                 VkDeviceMemory* imageMemory);

void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);

#endif
