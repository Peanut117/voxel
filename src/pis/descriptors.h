#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "volk.h"

typedef struct Descriptor {
    uint32_t bindingCount;
    VkDescriptorSetLayoutBinding* binding;
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    VkDescriptorSet* sets;
} Descriptor;

void CreateDescriptorSetLayout(VkDevice device, Descriptor* descriptor);

void CreateDescriptorPool(VkDevice device, Descriptor* descriptor, uint32_t setCount);

void AllocateDescriptorSet(VkDevice device, Descriptor* descriptor);


#endif
