#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "engine.h"

#include "vulkan/buffers.h"
#include "vulkan/descriptors.h"
#include "pisVoxReader.h"
#include "vulkan/swapchain.h"
#include "vulkan/validationlayers.h"
#include "vulkan/initializers.h"
#include "vulkan/pipelines.h"
#include "vulkan/descriptors.h"
#include "vulkan/images.h"
#include "vulkan/pisdef.h"

#include "vulkan/volk.h"
#include "vulkan/vulkan_core.h"

/* ===================================Functions==================================== */
void PisEngineInitialize(PisEngine* pis);
void PisEngineDraw(PisEngine* pis);
void PisEngineCleanup(PisEngine* pis);

// Pis engine intialize functions
void InitWindow(PisEngine* pis, uint32_t windowWidth, uint32_t windowHeight);
void InitVulkan(PisEngine* pis);
void InitVoxelData(PisEngine* pis);
void InitDrawImage(PisEngine* pis);
void InitPaletteBuffer(PisEngine* pis);
void InitUniformBuffers(PisEngine* pis);
void InitVoxelBuffer(PisEngine* pis);

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

    if(pis->windowExtent.width != 0)
        InitWindow(pis, pis->windowExtent.width, pis->windowExtent.height);
    else
        InitWindow(pis, 832, 624);


    InitVulkan(pis);

    InitVoxelData(pis);

    InitDrawImage(pis);

    InitPaletteBuffer(pis);

    InitUniformBuffers(pis);

    InitCommands(pis);

    InitVoxelBuffer(pis);

    InitDescriptors(pis);

    InitSyncStructures(pis);

    InitPipeline(pis);
}

void UpdateUniformBuffer(PisEngine* pis, UniformBufferObject ubo)
{
    memcpy(pis->vk.uboBuffer.ptr, &ubo, sizeof(UniformBufferObject));
}

void PisEngineDraw(PisEngine* pis)
{
    int currentFrame = pis->frameNumber % pis->vk.swapchainImageCount;
    FrameData currentFrameData = pis->vk.frames[currentFrame];

    // Wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(pis->vk.device, 1, &currentFrameData.renderFence, true, UINT64_MAX));
    VK_CHECK(vkResetFences(pis->vk.device, 1, &currentFrameData.renderFence));

    // Request image from the swapchain
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(pis->vk.device, pis->vk.swapchain, UINT64_MAX,
                                   currentFrameData.swapchainSemaphore, VK_NULL_HANDLE, &swapchainImageIndex));

    VkCommandBuffer cmd = currentFrameData.mainCommandBuffer;

    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    pis->vk.drawExtent.width = pis->vk.drawImage.extent.width;
    pis->vk.drawExtent.height = pis->vk.drawImage.extent.height;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // Make the swapchain image into writable mode before rendering
    TransitionImage(cmd, pis->vk.drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // Make the voxel data image writeable

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pis->vk.compute.pipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pis->vk.compute.layout, 0, 1,
                            &pis->vk.descriptor.set, 0, NULL);

    // DrawBackground(cmd, pis);

    vkCmdDispatch(cmd, pis->vk.drawImage.extent.width / 16, pis->vk.drawImage.extent.height / 16, 1);

    // Make the image presentable
    TransitionImage(cmd, pis->vk.drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    TransitionImage(cmd, pis->vk.swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyImageToImage(cmd, pis->vk.drawImage.image, pis->vk.swapchainImages[swapchainImageIndex],
                     pis->vk.drawExtent, pis->vk.swapchainExtent);

    TransitionImage(cmd, pis->vk.swapchainImages[swapchainImageIndex],
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    // VkSubmitInfo submit = SubmitInfo(cmd, currentFrameData.renderSemaphore, currentFrameData.swapchainSemaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    //
    VkPipelineStageFlags stageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &currentFrameData.swapchainSemaphore,
        .pWaitDstStageMask = &stageFlag,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &currentFrameData.renderSemaphore
    };

    VK_CHECK(vkQueueSubmit(pis->vk.computeQueue, 1, &submit, currentFrameData.renderFence));

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.pSwapchains = &pis->vk.swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &currentFrameData.renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    // clock_t begin = clock();

    VK_CHECK(vkQueuePresentKHR(pis->vk.computeQueue, &presentInfo));

    // clock_t end = clock();
    // double timeSpent = (double)(end - begin) / CLOCKS_PER_SEC;
    // printf("Queue present: %f\n", timeSpent);
    // begin = clock();

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

    vkDestroyBuffer(device, pis->vk.paletteBuffer.buffer, NULL);
    vkFreeMemory(device, pis->vk.paletteBuffer.memory, NULL);

    vkDestroyBuffer(device, pis->vk.voxelBuffer.buffer, NULL);
    vkFreeMemory(device, pis->vk.voxelBuffer.memory, NULL);

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

void InitVoxelData(PisEngine* pis)
{
    pis->voxelData = PisVoxReadFromFile(pis->voxelFile);
}

void InitDrawImage(PisEngine* pis)
{
    pis->vk.frames = malloc(sizeof(FrameData) * pis->vk.swapchainImageCount);
    CreateDrawImages(pis, pis->windowExtent.width, pis->windowExtent.height);
}

void InitVoxelBuffer(PisEngine* pis)
{
    VkDeviceSize voxelDataSize = 256*256*256;

    VkDeviceSize bufferSize = voxelDataSize;
    CreateBuffer(pis->vk.device, pis->vk.physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &pis->vk.voxelBuffer);

    pis->vk.voxelBuffer.ptr = malloc(bufferSize);

    VK_CHECK(vkMapMemory(pis->vk.device, pis->vk.voxelBuffer.memory, 0, voxelDataSize, 0, &pis->vk.voxelBuffer.ptr));
        memcpy(pis->vk.voxelBuffer.ptr, pis->voxelData.voxels, (size_t)bufferSize);
    vkUnmapMemory(pis->vk.device, pis->vk.voxelBuffer.memory);
}

void InitPaletteBuffer(PisEngine* pis)
{
    VkDeviceSize bufferSize = sizeof(Material) * 256;
    CreateBuffer(pis->vk.device, pis->vk.physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &pis->vk.paletteBuffer);

    pis->vk.paletteBuffer.ptr = malloc(bufferSize);
    
    VK_CHECK(vkMapMemory(pis->vk.device, pis->vk.paletteBuffer.memory, 0, bufferSize, 0, &pis->vk.paletteBuffer.ptr));
        memcpy(pis->vk.paletteBuffer.ptr, pis->voxelData.materials, (size_t)bufferSize);
    vkUnmapMemory(pis->vk.device, pis->vk.paletteBuffer.memory);
}

void InitUniformBuffers(PisEngine* pis)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    CreateBuffer(pis->vk.device, pis->vk.physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &pis->vk.uboBuffer);

    pis->vk.uboBuffer.ptr = malloc(sizeof(UniformBufferObject));

    VK_CHECK(vkMapMemory(pis->vk.device, pis->vk.uboBuffer.memory, 0, bufferSize, 0, &pis->vk.uboBuffer.ptr));

    UniformBufferObject ubo = {0};
    UpdateUniformBuffer(pis, ubo);
}

void InitDescriptors(PisEngine* pis)
{
    VkDescriptorSetLayoutBinding descriptorLayouts[4] = {0};

    descriptorLayouts[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorLayouts[0].binding = 0;
    descriptorLayouts[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorLayouts[0].descriptorCount = 1;

    descriptorLayouts[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorLayouts[1].binding = 1;
    descriptorLayouts[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorLayouts[1].descriptorCount = 1;

    descriptorLayouts[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorLayouts[2].binding = 2;
    descriptorLayouts[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorLayouts[2].descriptorCount = 1;

    descriptorLayouts[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorLayouts[3].binding = 3;
    descriptorLayouts[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    descriptorLayouts[3].descriptorCount = 1;

    CreateDescriptorSetLayout(pis->vk.device, &pis->vk.descriptor.layout, descriptorLayouts, 4);

    // Pools
    VkDescriptorPoolSize poolSizes[3];

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[0].descriptorCount = 1;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = 2;

    poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[2].descriptorCount = 1;

    CreateDescriptorPool(pis->vk.device, &pis->vk.descriptor.pool, poolSizes, 3, 1);

    AllocateDescriptorSets(pis->vk.device, &pis->vk.descriptor);

    VkWriteDescriptorSet writeSets[4] = {0};

    VkDescriptorImageInfo drawImgInfo = {0};
    drawImgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    drawImgInfo.imageView = pis->vk.drawImage.view;
    drawImgInfo.sampler = VK_NULL_HANDLE;

    writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[0].dstSet = pis->vk.descriptor.set;
    writeSets[0].dstBinding = 0;
    writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeSets[0].pImageInfo = &drawImgInfo;
    writeSets[0].descriptorCount = 1;

    VkDescriptorBufferInfo voxelBufferInfo = {0};
    voxelBufferInfo.buffer = pis->vk.voxelBuffer.buffer;
    voxelBufferInfo.offset = 0;
    voxelBufferInfo.range = pis->vk.voxelBuffer.size;

    writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[1].dstSet = pis->vk.descriptor.set;
    writeSets[1].dstBinding = 1;
    writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeSets[1].pBufferInfo = &voxelBufferInfo;
    writeSets[1].descriptorCount = 1;

    VkDescriptorBufferInfo paletteBufferInfo = {0};
    paletteBufferInfo.buffer = pis->vk.paletteBuffer.buffer;
    paletteBufferInfo.offset = 0;
    paletteBufferInfo.range = pis->vk.paletteBuffer.size;

    writeSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[2].dstSet = pis->vk.descriptor.set;
    writeSets[2].dstBinding = 2;
    writeSets[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeSets[2].pBufferInfo = &paletteBufferInfo;
    writeSets[2].descriptorCount = 1;

    VkDescriptorBufferInfo uboInfo = {0};
    uboInfo.buffer = pis->vk.uboBuffer.buffer;
    uboInfo.offset = 0;
    uboInfo.range = sizeof(UniformBufferObject);

    writeSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[3].dstSet = pis->vk.descriptor.set;
    writeSets[3].dstBinding = 3;
    writeSets[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeSets[3].pBufferInfo = &uboInfo;
    writeSets[3].descriptorCount = 1;

    vkUpdateDescriptorSets(pis->vk.device, 4, writeSets, 0, NULL);
}

void InitCommands(PisEngine* pis)
{
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
    clearValue = (VkClearColorValue){ { 0.f, 0.f, 0.f, 1.f } };

    VkImageSubresourceRange clearRange = ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

    // Clear image
    vkCmdClearColorImage(cmd, pis->vk.drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

}
