#ifndef DEFINE_H
#define DEFINE_H

#include <stdio.h>
#include <vulkan/vk_enum_string_helper.h>
#include <cglm/cglm.h>

/********************Structs*******************/
typedef struct UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} UniformBufferObject;

/********************Defines*******************/
#define VK_CHECK(result) \
	if(result != VK_SUCCESS) { \
		fprintf(stderr, "Vulkan error: %s\n", string_VkResult(result)); \
		return -1; \
	}

#endif
