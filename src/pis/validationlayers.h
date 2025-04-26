#ifdef DEBUG
#ifndef VALIDATION_LAYER_H
#define VALIDATION_LAYER_H

#include <stdbool.h>
#include "volk.h"

typedef struct {
    uint32_t count;
    const char* layers[];
} ValidationLayer;

uint32_t GetValidationLayerCount();

const char** GetValidationLayers();

bool CheckValidationLayerSupport();

void SetupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* debugMessenger, bool intoFile);

void DestroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);

#endif
#endif
