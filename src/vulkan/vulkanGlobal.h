#ifndef VULKAN_GLOBAL_H
#define VULKAN_GLOBAL_H

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>

#include <SDL3/SDL.h>

#include <cglm/cglm.h>

/**************Global structs***************/
typedef struct swapchainSupportDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	uint32_t presentModeCount;
	VkPresentModeKHR* presentModes;
	uint32_t surfaceFormatCount;
	VkSurfaceFormatKHR* surfaceFormats;
} swapchainSupportDetails;

typedef struct Vertex {
	vec3 pos;
} Vertex;

typedef struct QueueFamilyIndices {
	uint32_t graphicsFamilyIndex;
	bool graphicsFamilyIsSet;
} QueueFamilyIndices;

typedef struct swapchainImageDetails {
	uint32_t imageCount;
	VkFormat format;
	VkExtent2D extent;
} swapchainImageDetails;

/**************Global functions***************/
int createSwapchain(void);
int createDepthResources(void);
int createFramebuffers(void);

int cleanupSwapchain(void);

swapchainSupportDetails gatherSwapchainSupportDetails(VkPhysicalDevice device); //mallocates 2 heaps
void cleanupSwapchainSupportDetails(swapchainSupportDetails details); //Frees these heaps i hope
QueueFamilyIndices gatherQueueFamilies(VkPhysicalDevice device);

/**************Global variables***************/
extern SDL_Window* window;
extern VkPhysicalDevice physicalDevice;
extern VkSurfaceKHR surface;
extern VkDevice device;
extern VkQueue graphicsQueue;
extern VkSwapchainKHR swapchain;
extern VkCommandBuffer* commandBuffers;
extern VkRenderPass renderPass;
extern VkFramebuffer* framebuffers;
extern VkPipelineLayout graphicsPipelineLayout;
extern VkPipeline graphicsPipeline;
extern VkBuffer vertexBuffer;
extern VkBuffer indexBuffer;
extern VkDescriptorSet* descriptorSets;
extern void* uniformBufferPtr[3];
extern VkImageView* swapchainImageViews;
extern VkImage depthImage;
extern VkDeviceMemory depthImageMemory;
extern VkImageView depthImageView;


extern VkSemaphore* imageAvailableSemaphores;
extern VkSemaphore* renderFinishedSemaphores;
extern VkFence* inFlightFences;

extern swapchainImageDetails swapchainDetails;

static const Vertex vertices[] = {
	{{-1.0f, -1.0f, 0.0f}},
    {{1.0f, -1.0f, 0.0f}},
    {{1.0f, 1.0f, 0.0f}},
    {{-1.0f, 1.0f, 0.0f}},
};

static const size_t vertexCount = sizeof(vertices) / sizeof(Vertex);

static const uint16_t indices[] = {
	0, 1, 2, 2, 3, 0,
};

#endif
