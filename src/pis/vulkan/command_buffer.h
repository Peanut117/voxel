#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include "volk.h"

VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice device, VkCommandPool commandPool, VkQueue queue);

#endif
