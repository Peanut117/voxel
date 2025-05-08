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

VkSubmitInfo SubmitInfo(VkCommandBuffer cmd, VkSemaphore signalSemaphore, VkSemaphore waitSemaphore, VkPipelineStageFlags waitStage);

VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageType imageType, VkImageUsageFlags usageFlags, VkExtent3D extent);
VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImageViewType imageType, VkImage image, VkImageAspectFlags aspectFlags);

VkBufferImageCopy BufferImageCopyInfo(VkImageAspectFlags aspectMask, VkExtent3D extent);

#endif
