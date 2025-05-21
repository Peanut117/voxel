#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "volk.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>

typedef struct Descriptor {
    uint32_t bindingCount;
    VkDescriptorSetLayoutBinding binding;
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    VkDescriptorSet set;
} Descriptor;

typedef struct PoolSize {
    VkDescriptorType type;
    float ratio;
} PoolSize;

void CreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout* layout, VkDescriptorSetLayoutBinding* descriptorLayoutBindings, uint32_t bindingCount);

void CreateDescriptorPool(VkDevice device, VkDescriptorPool* pool, VkDescriptorPoolSize* poolSizes, uint32_t poolSizeCount, uint32_t maxSets);

void AllocateDescriptorSets(VkDevice device, Descriptor* descriptor);

#endif
