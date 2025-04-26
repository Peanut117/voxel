#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>

#include "volk.h"
#include "vulkan/vulkan_core.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "pipelines.h"
#include "descriptors.h"
#include "images.h"

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

enum { FRAME_OVERLAP = 2 };

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

    Descriptor drawImageDescriptor;

    FrameData frames[FRAME_OVERLAP];

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
} PisEngine;

void PisEngineInitialize(PisEngine* pis);

void PisEngineDraw(PisEngine* pis);

void PisEngineCleanup(PisEngine* pis);

#endif
