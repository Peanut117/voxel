#ifndef BUFFERS_H
#define BUFFERS_H

#include "volk.h"
#include "vulkan/vulkan_core.h"

typedef struct Buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* ptr;
    VkDeviceSize size;
} Buffer;

int CreateBuffer(VkDevice device, VkPhysicalDevice pDevice,
                 VkDeviceSize size, VkBufferUsageFlags usage,
                 VkMemoryPropertyFlags properties, Buffer* buffer);

uint32_t FindMemoryType(VkPhysicalDevice pDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

#endif
