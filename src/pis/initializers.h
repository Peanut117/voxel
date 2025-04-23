#ifndef INITIALIZERS_H
#define INITIALIZERS_H

#include "volk.h"
#include "vulkan/vulkan_core.h"

VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags);
VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags);

VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags);

VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);

VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);

VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd,
                         VkSemaphoreSubmitInfo* signalSemaphoreInfo,
                         VkSemaphoreSubmitInfo* waitSemaphoreInfo);

VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

#endif
