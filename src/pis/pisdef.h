#ifndef PISDEF_H
#define PISDEF_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "volk.h"

#include <cglm/cglm.h>

#ifdef DEBUG
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(result) \
	if(result != VK_SUCCESS) { \
		fprintf(stderr, "Vulkan error: %s\n", string_VkResult(result)); \
		exit(-1); \
	}
#else
#define VK_CHECK(result) result
#endif

#endif
