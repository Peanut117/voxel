#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>

#include "vulkan/volk.h"
#include "vulkan/vulkan_core.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "vulkan/pipelines.h"
#include "vulkan/descriptors.h"
#include "vulkan/images.h"
#include "vulkan/buffers.h"

#include "pisVoxReader.h"

#include "cglm/cglm.h"

typedef struct UniformBufferObject {
    vec3 position;  float _pad1;
    vec3 forward;   float _pad2;
    vec3 right;     float _pad3;
    vec3 up;

    float fov;
    float time;
} UniformBufferObject;

typedef struct QueueFamilyIndices {
    uint32_t computeFamilyIndex;
    bool computeFamilyIsAvailable;
} QueueFamilyIndices;

typedef struct FrameData {
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;

    VkSemaphore swapchainSemaphore, renderSemaphore;
    VkFence renderFence;
} FrameData;

typedef struct PisVulkanInstance {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkColorSpaceKHR swapchainColorSpace;

    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    uint32_t swapchainImageCount;
    VkExtent2D swapchainExtent;

    AllocatedImage drawImage;
    VkExtent2D drawExtent;

    QueueFamilyIndices indices;
    VkQueue computeQueue;

    Pipeline compute;

    Descriptor descriptor;

    Buffer uboBuffer;
    Buffer voxelBuffer;
    Buffer paletteBuffer;

    FrameData* frames;

    #ifdef DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
    #endif
} PisVulkanInstance;

typedef struct PisEngine {
    SDL_Window* window;
    PisVulkanInstance vk;
    uint32_t frameNumber;
    bool stopRendering;
    VkExtent2D windowExtent;
    char voxelFile[128];
    PisVox voxelData;
} PisEngine;

void PisEngineInitialize(PisEngine* pis);

void PisEngineDraw(PisEngine* pis);

void PisEngineCleanup(PisEngine* pis);

void UpdateUniformBuffer(PisEngine* pis, UniformBufferObject ubo);

#endif
