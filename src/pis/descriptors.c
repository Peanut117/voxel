#include "descriptors.h"
#include "pisdef.h"
#include "vulkan/vulkan_core.h"
#include <stdlib.h>

void CreateDescriptorSetLayout(VkDevice device, Descriptor* descriptor)
{
    // Make a function to add these easily
    descriptor->binding = malloc(sizeof(VkDescriptorSetLayoutBinding));
    descriptor->bindingCount = 1;

	descriptor->binding[0].binding = 0;
	descriptor->binding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptor->binding[0].descriptorCount = 1;
	descriptor->binding[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = descriptor->bindingCount,
		.pBindings = descriptor->binding
	};

	VK_CHECK(vkCreateDescriptorSetLayout(device, &createInfo, NULL, &descriptor->layout));
}

void CreateDescriptorPool(VkDevice device, Descriptor* descriptor, uint32_t setCount)
{
    enum { poolSizeCount = 1 };
	VkDescriptorPoolSize poolSizes[poolSizeCount] = {
	{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = setCount
		}
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = setCount,
		.poolSizeCount = poolSizeCount,
		.pPoolSizes = poolSizes
	};

	VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolInfo, NULL, &descriptor->pool));
}

void AllocateDescriptorSet(VkDevice device, Descriptor* descriptor)
{
    // Oei oei oei, dit moet wat netter
    descriptor->sets = malloc(sizeof(VkDescriptorSet));

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = NULL;
    allocInfo.descriptorPool = descriptor->pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptor->layout;

    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptor->sets[0]));
}
