#ifndef PIPELINES_H
#define PIPELINES_H

#include "volk.h"

typedef struct Pipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
} Pipeline;

void CreateComputePipelineLayout(VkDevice device, VkDescriptorSetLayout* descriptorLayouts, uint32_t descriptorLayoutCount, VkPipelineLayout* layout);

void CreateComputePipeline(VkDevice device, VkPipelineLayout layout, VkPipeline* computePipeline);

#endif
