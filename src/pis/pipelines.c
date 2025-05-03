#include "pipelines.h"

#include "pisdef.h"
#include "vulkan/vulkan_core.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

VkShaderModule CreateShaderModule(VkDevice device, const char* fileName)
{
    int fd = open(fileName, O_RDONLY);
	struct stat sb;

	if(fd == -1)
	{
		perror("No file :(\n");
		return NULL;
	}

	if(fstat(fd, &sb) == -1)
	{
		perror("Failed to fstat file\n");
		return NULL;
	}

	size_t size = sb.st_size;

	char* code = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	//close(fd);

	uint32_t* codeConverted = (uint32_t*)code;

	VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = codeConverted
	};

	VkShaderModule shaderModule;
	if(vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create shader module: %s\n", fileName);
	}

	return shaderModule;
}

void CreateComputePipelineLayout(VkDevice device, VkDescriptorSetLayout* descriptorLayouts, uint32_t descriptorLayoutCount, VkPipelineLayout* layout)
{
    VkPipelineLayoutCreateInfo layoutCreateInfo = {0};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.pNext = NULL;
    layoutCreateInfo.setLayoutCount = descriptorLayoutCount;
    layoutCreateInfo.pSetLayouts = descriptorLayouts;

    VK_CHECK(vkCreatePipelineLayout(device,
                                    &layoutCreateInfo,
                                    NULL,
                                    layout));
}

void CreateComputePipeline(VkDevice device, VkPipelineLayout layout, VkPipeline* computePipeline)
{
    VkShaderModule computeShaderMod = CreateShaderModule(device, "/Users/nielsbil/Dev/voxel/shaders/gradient.spv");

    VkPipelineShaderStageCreateInfo shaderStage = {0};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.pNext = NULL;
    shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module = computeShaderMod;
    shaderStage.pName = "main";

    VkComputePipelineCreateInfo pipelineCreateInfo = {0};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = NULL;
    pipelineCreateInfo.stage = shaderStage;
    pipelineCreateInfo.layout = layout;

    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE,
                                      1, &pipelineCreateInfo,
                                      NULL, computePipeline));

    vkDestroyShaderModule(device, computeShaderMod, NULL);
}
