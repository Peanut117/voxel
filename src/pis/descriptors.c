#include "descriptors.h"
#include "pisdef.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <stdlib.h>

void CreateDescriptorSetLayout(VkDevice device, Descriptor* descriptor, DescriptorLayout* descriptorLayout, uint32_t layoutCount)
{
    // Make a function to add these easily
    descriptor->binding = malloc(sizeof(VkDescriptorSetLayoutBinding) * layoutCount);
    descriptor->bindingCount = layoutCount;

    for(uint32_t i = 0; i < layoutCount; i++)
    {
        descriptor->binding[i].binding = descriptorLayout[i].binding;
        descriptor->binding[i].descriptorType = descriptorLayout[i].type;
        descriptor->binding[i].descriptorCount = 1;
        descriptor->binding[i].stageFlags = descriptorLayout[i].flags;
    }

    VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = descriptor->bindingCount,
        .pBindings = descriptor->binding
    };

    VK_CHECK(vkCreateDescriptorSetLayout(device, &createInfo, NULL, &descriptor->layout));
}

void CreateDescriptorPool(VkDevice device, Descriptor* descriptor, PoolSize* poolSize, uint32_t poolSizeCount, uint32_t maxSets)
{
    VkDescriptorPoolSize poolSizes[poolSizeCount];
    for(uint32_t i = 0; i < poolSizeCount; i++)
    {
        poolSizes[i].type = poolSize[i].type;
        poolSizes[i].descriptorCount = (maxSets * poolSize[i].ratio);
    }

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = maxSets,
        .poolSizeCount = poolSizeCount,
        .pPoolSizes = poolSizes
    };

    VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolInfo, NULL, &descriptor->pool));
}

void AllocateDescriptorSets(VkDevice device, Descriptor* descriptor, uint32_t descriptorSetCount)
{
    // Oei oei oei, dit moet wat netter
    descriptor->sets = malloc(sizeof(VkDescriptorSet) * descriptorSetCount);

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = NULL;
    allocInfo.descriptorPool = descriptor->pool;
    allocInfo.descriptorSetCount = descriptorSetCount;
    VkDescriptorSetLayout layouts[descriptorSetCount];
    for(uint32_t i = 0; i < descriptorSetCount; i++)
    {
        layouts[i] = descriptor->layout;
    }
    allocInfo.pSetLayouts = layouts;

    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, descriptor->sets));
}
