#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"

#include "validationlayers.h"
#include "initializers.h"
#include "images.h"
#include "pisdef.h"

#include "volk.h"
#include "vulkan/vulkan_core.h"

/* ===================================Functions==================================== */
void PisEngineInitialize(PisEngine* pis);
void PisEngineDraw(PisEngine* pis);
void PisEngineCleanup(PisEngine* pis);

// Pis engine intialize functions
void InitWindow(PisEngine* pis, uint32_t windowWidth, uint32_t windowHeight);
void InitVulkan(PisEngine* pis);
void InitCommands(PisEngine* pis);
void InitSyncStructures(PisEngine* pis);

/* =================================Helper functions================================ */
void DrawBackground(VkCommandBuffer cmd, PisEngine* pis);
/* ================================================================================ */

void PisEngineInitialize(PisEngine* pis)
{
    // Set pis variables
    pis->frameNumber = 0;
    pis->stopRendering = false;

    InitWindow(pis, 1920, 1080);

    InitVulkan(pis);

    InitCommands(pis);

    InitSyncStructures(pis);
}

void PisEngineDraw(PisEngine* pis)
{
    FrameData currentFrame = pis->vk.frames[pis->frameNumber % FRAME_OVERLAP];

    // Wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(pis->vk.device, 1, &currentFrame.renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(pis->vk.device, 1, &currentFrame.renderFence));

    // Request image from the swapchain
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(pis->vk.device, pis->vk.swapchain, 1000000000,
                                   currentFrame.swapchainSemaphore, NULL, &swapchainImageIndex));

    VkCommandBuffer cmd = currentFrame.mainCommandBuffer;

    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    pis->vk.drawExtent.width = pis->vk.drawImage.extent.width;
    pis->vk.drawExtent.height = pis->vk.drawImage.extent.height;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // Make the swapchain image into writable mode before rendering
    TransitionImage(cmd, pis->vk.drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    DrawBackground(cmd, pis);

    // Make the image presentable
    TransitionImage(cmd, pis->vk.drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    TransitionImage(cmd, pis->vk.swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyImageToImage(cmd, pis->vk.drawImage.image, pis->vk.swapchainImages[swapchainImageIndex],
                     pis->vk.drawExtent, pis->vk.swapchainExtent);

    TransitionImage(cmd, pis->vk.swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo = CommandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, currentFrame.swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrame.renderSemaphore);

    VkSubmitInfo2 submit = SubmitInfo(&cmdInfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(pis->vk.computeQueue, 1, &submit, currentFrame.renderFence));

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.pSwapchains = &pis->vk.swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &currentFrame.renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(pis->vk.computeQueue, &presentInfo));

    pis->frameNumber++;
}

void PisEngineCleanup(PisEngine* pis)
{
    vkDeviceWaitIdle(pis->vk.device);

    for(uint32_t i = 0; i < FRAME_OVERLAP; i++)
    {
        vkDestroyFence(pis->vk.device, pis->vk.frames[i].renderFence, NULL);
        vkDestroySemaphore(pis->vk.device, pis->vk.frames[i].swapchainSemaphore, NULL);
        vkDestroySemaphore(pis->vk.device, pis->vk.frames[i].renderSemaphore, NULL);
        vkDestroyCommandPool(pis->vk.device, pis->vk.frames[i].commandPool, NULL);
    }

    vkDestroySwapchainKHR(pis->vk.device, pis->vk.swapchain, NULL);
    for(uint32_t i = 0; i < pis->vk.swapchainImageCount; i++)
    {
        vkDestroyImageView(pis->vk.device, pis->vk.swapchainImageViews[i], NULL);
    }

    free(pis->vk.swapchainImages);
    free(pis->vk.swapchainImageViews);

    vkDestroyDevice(pis->vk.device, NULL);

    vkDestroySurfaceKHR(pis->vk.instance, pis->vk.surface, NULL);

#ifdef DEBUG
    DestroyDebugUtilsMessenger(pis->vk.instance, pis->vk.debugMessenger);
#endif

    vkDestroyInstance(pis->vk.instance, NULL);

    SDL_DestroyWindow(pis->window);
    SDL_Quit();
}

void InitWindow(PisEngine* pis, uint32_t windowWidth, uint32_t windowHeight)
{
    // Set window extent
    pis->windowExtent.width = windowWidth;
    pis->windowExtent.height = windowHeight;

    // Initialize SDL3 and create a window with it
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags windowFlags = (SDL_WindowFlags){SDL_WINDOW_VULKAN};

    pis->window = SDL_CreateWindow(
        "Voxel",
        windowWidth,
        windowHeight,
        windowFlags
    );
}

void InitCommands(PisEngine* pis)
{
    //create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers

    VkCommandPoolCreateInfo commandPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = pis->vk.indices.computeFamilyIndex,
    };

    for(uint32_t i = 0; i < FRAME_OVERLAP; i++)
    {
        VK_CHECK(vkCreateCommandPool(pis->vk.device, &commandPoolInfo, NULL, &pis->vk.frames[i].commandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = pis->vk.frames[i].commandPool,
            .commandBufferCount = 1,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
        };

        VK_CHECK(vkAllocateCommandBuffers(pis->vk.device, &cmdAllocInfo, &pis->vk.frames[i].mainCommandBuffer));
    }
}

void InitSyncStructures(PisEngine* pis)
{
    //create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame
    VkFenceCreateInfo fenceCreateInfo = FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = SemaphoreCreateInfo(0);

    for(uint32_t i = 0; i < FRAME_OVERLAP; i++)
    {
        VK_CHECK(vkCreateFence(pis->vk.device, &fenceCreateInfo, NULL, &pis->vk.frames[i].renderFence));

        VK_CHECK(vkCreateSemaphore(pis->vk.device, &semaphoreCreateInfo, NULL, &pis->vk.frames[i].swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(pis->vk.device, &semaphoreCreateInfo, NULL, &pis->vk.frames[i].renderSemaphore));
    }
}

void DrawBackground(VkCommandBuffer cmd, PisEngine* pis)
{
    // Make a clear color from frame number
    VkClearColorValue clearValue;
    float flash = fabs(sin(pis->frameNumber / 120.f));
    clearValue = (VkClearColorValue){ { 0.f, 0.f, flash, 1.f } };

    VkImageSubresourceRange clearRange = ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

    // Clear image
    vkCmdClearColorImage(cmd, pis->vk.drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

}
