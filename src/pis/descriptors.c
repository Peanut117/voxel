#include "descriptors.h"
#include "pisdef.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <stdlib.h>

void CreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout* layout, VkDescriptorSetLayoutBinding* descriptorLayoutBindings, uint32_t bindingCount)
{
    VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = bindingCount,
        .pBindings = descriptorLayoutBindings
    };

    VK_CHECK(vkCreateDescriptorSetLayout(device, &createInfo, NULL, layout));
}

void CreateDescriptorPool(VkDevice device, VkDescriptorPool* pool, VkDescriptorPoolSize* poolSizes, uint32_t poolSizeCount, uint32_t maxSets)
{
    VkDescriptorPoolCreateInfo descriptorPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = poolSizeCount,
        .pPoolSizes = poolSizes,
        .maxSets = maxSets,
    };

    VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolInfo, NULL, pool));
}

void AllocateDescriptorSets(VkDevice device, Descriptor* descriptor)
{
    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = NULL;
    allocInfo.descriptorPool = descriptor->pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptor->layout;

    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptor->set));
}
