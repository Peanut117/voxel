#include "validationlayers.h"

#include "pisdef.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =====================Add needed validation layers here========================= */
ValidationLayer validationLayer = {
    .count = 1,
    .layers = {
        "VK_LAYER_KHRONOS_validation"
    }
};
/* =============================================================================== */

FILE* validationDebugFile = NULL;

uint32_t GetValidationLayerCount()
{
    return validationLayer.count;
}

const char** GetValidationLayers()
{
    return validationLayer.layers;
}

bool CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for(uint32_t i = 0; i < validationLayer.count; i++)
    {
        bool layerFound = false;
        for(uint32_t j = 0; j < layerCount; j++)
        {
            if(strcmp(availableLayers[j].layerName, validationLayer.layers[i]) == 0)
            {
                layerFound = true;
            }
        }

        if(!layerFound)
        {
            return false;
        }
    }

    return true;
}

// Setup the debug callback for vulkan validation layers
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        fprintf(validationDebugFile, "validation layer ERROR:\n%s\n\n", pCallbackData->pMessage);
    else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        fprintf(validationDebugFile, "validation layer WARNING:\n%s\n\n", pCallbackData->pMessage);
    else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        fprintf(validationDebugFile, "validation layer INFO:\n%s\n\n", pCallbackData->pMessage);
    else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        fprintf(validationDebugFile, "validation layer VERBOSE:\n%s\n\n", pCallbackData->pMessage);

    return VK_FALSE;
}

void SetupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* debugMessenger)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = NULL
    };

    VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &createInfo, NULL, debugMessenger));

    validationDebugFile = fopen("ValidationLayer.txt", "w");
    if(validationDebugFile == NULL)
    {
        fprintf(stderr, "Failed to open file\n");
        exit(-1);
    }
}

void DestroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger)
{
    fclose(validationDebugFile);
    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
}
