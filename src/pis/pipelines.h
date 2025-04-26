#ifndef PIPELINES_H
#define PIPELINES_H

#include "volk.h"

#include "descriptors.h"

typedef struct Pipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
} Pipeline;

void CreateComputePipelineLayout(VkDevice device, Descriptor* descriptor, VkPipelineLayout* layout);

void CreateComputePipeline(VkDevice device, VkPipelineLayout layout, VkPipeline* computePipeline);

#endif
