#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"

#include "buffers.h"
#include "descriptors.h"
#include "swapchain.h"
#include "validationlayers.h"
#include "initializers.h"
#include "pipelines.h"
#include "descriptors.h"
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
void InitDrawImage(PisEngine* pis);
void InitUniformBuffers(PisEngine* pis);
void InitDescriptors(PisEngine* pis);
void InitCommands(PisEngine* pis);
void InitSyncStructures(PisEngine* pis);
void InitBuffers(PisEngine* pis);
void InitPipeline(PisEngine* pis);

/* =================================Helper functions================================ */
void DrawBackground(VkCommandBuffer cmd, PisEngine* pis);
/* ================================================================================ */

void PisEngineInitialize(PisEngine* pis)
{
    // Set pis variables
    pis->frameNumber = 0;
    pis->stopRendering = false;

    InitWindow(pis, 800, 600);

    InitVulkan(pis);

    InitDrawImage(pis);

    InitUniformBuffers(pis);

    InitDescriptors(pis);

    InitCommands(pis);

    InitSyncStructures(pis);

    InitPipeline(pis);
}

void UpdateUniformBuffer(PisEngine* pis)
{
    pis->ubo.time = (float)SDL_GetTicks() / 1000.f;

    printf("%.1f\n", pis->ubo.time);

    memcpy(pis->vk.uboBuffer.ptr, &pis->ubo, sizeof(UniformBufferObject));
}

void PisEngineDraw(PisEngine* pis)
{
    int currentFrame = pis->frameNumber % pis->vk.swapchainImageCount;
    FrameData currentFrameData = pis->vk.frames[currentFrame];

    UpdateUniformBuffer(pis);

    // Wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(pis->vk.device, 1, &currentFrameData.renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(pis->vk.device, 1, &currentFrameData.renderFence));

    // Request image from the swapchain
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(pis->vk.device, pis->vk.swapchain, 1000000000,
                                   currentFrameData.swapchainSemaphore, NULL, &swapchainImageIndex));

    VkCommandBuffer cmd = currentFrameData.mainCommandBuffer;

    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    pis->vk.drawExtent.width = pis->vk.drawImage.extent.width;
    pis->vk.drawExtent.height = pis->vk.drawImage.extent.height;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // Make the swapchain image into writable mode before rendering
    TransitionImage(cmd, pis->vk.drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pis->vk.compute.pipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pis->vk.compute.layout, 0, 1,
                            &pis->vk.descriptor.set, 0, NULL);

    vkCmdDispatch(cmd, pis->vk.drawImage.extent.width / 16, pis->vk.drawImage.extent.height / 16, 1);

    // Make the image presentable
    TransitionImage(cmd, pis->vk.drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    TransitionImage(cmd, pis->vk.swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyImageToImage(cmd, pis->vk.drawImage.image, pis->vk.swapchainImages[swapchainImageIndex],
                     pis->vk.drawExtent, pis->vk.swapchainExtent);

    TransitionImage(cmd, pis->vk.swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo = CommandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, currentFrameData.swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrameData.renderSemaphore);

    VkSubmitInfo2 submit = SubmitInfo(&cmdInfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(pis->vk.computeQueue, 1, &submit, currentFrameData.renderFence));

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.pSwapchains = &pis->vk.swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &currentFrameData.renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(pis->vk.computeQueue, &presentInfo));

    pis->frameNumber++;
}

void PisEngineCleanup(PisEngine* pis)
{
    VkDevice device = pis->vk.device;

    vkDeviceWaitIdle(device);

    vkDestroyDescriptorPool(device, pis->vk.descriptor.pool, NULL);

    vkDestroyDescriptorSetLayout(device, pis->vk.descriptor.layout, NULL);

    vkDestroyPipeline(device, pis->vk.compute.pipeline, NULL);
    vkDestroyPipelineLayout(device, pis->vk.compute.layout, NULL);

    vkDestroyBuffer(device, pis->vk.uboBuffer.buffer, NULL);
    vkFreeMemory(device, pis->vk.uboBuffer.memory, NULL);

    vkDestroyImageView(device, pis->vk.drawImage.view, NULL);
    vkDestroyImage(device, pis->vk.drawImage.image, NULL);
    vkFreeMemory(device, pis->vk.drawImage.memory, NULL);

    for(uint32_t i = 0; i < pis->vk.swapchainImageCount; i++)
    {
        vkDestroyFence(device, pis->vk.frames[i].renderFence, NULL);
        vkDestroySemaphore(device, pis->vk.frames[i].swapchainSemaphore, NULL);
        vkDestroySemaphore(device, pis->vk.frames[i].renderSemaphore, NULL);
        vkDestroyCommandPool(device, pis->vk.frames[i].commandPool, NULL);
    }

    free(pis->vk.frames);

    vkDestroySwapchainKHR(device, pis->vk.swapchain, NULL);
    for(uint32_t i = 0; i < pis->vk.swapchainImageCount; i++)
    {
        vkDestroyImageView(device, pis->vk.swapchainImageViews[i], NULL);
    }

    free(pis->vk.swapchainImages);
    free(pis->vk.swapchainImageViews);

    vkDestroyDevice(device, NULL);

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

void InitDrawImage(PisEngine* pis)
{
    pis->vk.frames = malloc(sizeof(FrameData) * pis->vk.swapchainImageCount);
    CreateDrawImages(pis, pis->windowExtent.width, pis->windowExtent.height);
}

void InitUniformBuffers(PisEngine* pis)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    CreateBuffer(pis->vk.device, pis->vk.physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &pis->vk.uboBuffer);

    pis->vk.uboBuffer.ptr = malloc(sizeof(UniformBufferObject));

    VK_CHECK(vkMapMemory(pis->vk.device, pis->vk.uboBuffer.memory, 0, bufferSize, 0, &pis->vk.uboBuffer.ptr));

    pis->ubo.time = 1.0f;
}

void InitDescriptors(PisEngine* pis)
{
    VkDescriptorSetLayoutBinding descriptorLayouts[2] = {0};

    descriptorLayouts[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorLayouts[0].binding = 0;
    descriptorLayouts[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorLayouts[0].descriptorCount = 1;

    descriptorLayouts[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorLayouts[1].binding = 1;
    descriptorLayouts[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorLayouts[1].descriptorCount = 1;

    CreateDescriptorSetLayout(pis->vk.device, &pis->vk.descriptor.layout, descriptorLayouts, 2);

    // Pools
    VkDescriptorPoolSize poolSizes[2];

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[0].descriptorCount = 1;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 1;

    CreateDescriptorPool(pis->vk.device, &pis->vk.descriptor.pool, poolSizes, 2, 1);

    AllocateDescriptorSets(pis->vk.device, &pis->vk.descriptor);

    VkWriteDescriptorSet writeSets[2];

    VkDescriptorImageInfo imgInfo = {0};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView = pis->vk.drawImage.view;

    writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[0].dstSet = pis->vk.descriptor.set;
    writeSets[0].dstBinding = 0;
    writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeSets[0].pImageInfo = &imgInfo;
    writeSets[0].descriptorCount = 1;

    VkDescriptorBufferInfo bufferInfo = {0};
    bufferInfo.buffer = pis->vk.uboBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[1].dstSet = pis->vk.descriptor.set;
    writeSets[1].dstBinding = 1;
    writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeSets[1].pBufferInfo = &bufferInfo;
    writeSets[1].descriptorCount = 1;

    vkUpdateDescriptorSets(pis->vk.device, 2, writeSets, 0, NULL);
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

    for(uint32_t i = 0; i < pis->vk.swapchainImageCount; i++)
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

    for(uint32_t i = 0; i < pis->vk.swapchainImageCount; i++)
    {
        VK_CHECK(vkCreateFence(pis->vk.device, &fenceCreateInfo, NULL, &pis->vk.frames[i].renderFence));

        VK_CHECK(vkCreateSemaphore(pis->vk.device, &semaphoreCreateInfo, NULL, &pis->vk.frames[i].swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(pis->vk.device, &semaphoreCreateInfo, NULL, &pis->vk.frames[i].renderSemaphore));
    }
}

void InitPipeline(PisEngine* pis)
{
    CreateComputePipelineLayout(pis->vk.device, &pis->vk.descriptor.layout, 1, &pis->vk.compute.layout);
    CreateComputePipeline(pis->vk.device, pis->vk.compute.layout, &pis->vk.compute.pipeline);
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
