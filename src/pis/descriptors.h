#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "volk.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>

typedef struct Descriptor {
    uint32_t bindingCount;
    VkDescriptorSetLayoutBinding* binding;
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    VkDescriptorSet* sets;
} Descriptor;

typedef struct DescriptorLayout {
    VkDescriptorType type;
    uint32_t binding;
    VkShaderStageFlags flags;
} DescriptorLayout;

typedef struct PoolSize {
    VkDescriptorType type;
    float ratio;
} PoolSize;

void CreateDescriptorSetLayout(VkDevice device, Descriptor* descriptor, DescriptorLayout* descriptorLayout, uint32_t layoutCount);

void CreateDescriptorPool(VkDevice device, Descriptor* descriptor, PoolSize* poolSize, uint32_t poolSizeCount, uint32_t maxSets);

void AllocateDescriptorSets(VkDevice device, Descriptor* descriptor, uint32_t descriptorSetCount);

#endif
