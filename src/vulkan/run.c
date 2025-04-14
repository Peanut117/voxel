#include <stdint.h>

#include <vulkan/vulkan_core.h>

#include "cglm/mat4.h"
#include "vulkanGlobal.h"
#include "run.h"

#include "global.h"

/********************Variables*******************/
uint32_t currentFrame = 0;
bool framebufferResized  = false;

UniformBufferObject ubo = {0};

/********************Function definitions*******************/
int	recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

int recreateSwapchain(void);
int cleanupSwapchain(void);
int updateUniformBuffer(uint32_t currentFrame);

int cleanupSwapchain(void);

/********************Functions*******************/
int drawFrame(void)
{
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame],VK_NULL_HANDLE, &imageIndex);
	
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
	{
		framebufferResized = false;
		recreateSwapchain();
		return 0;
	} else if(result != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to acquire swapchain image\n");
		return -1;
	}

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	vkResetCommandBuffer(commandBuffers[currentFrame], 0);

	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	updateUniformBuffer(currentFrame);

	VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffers[currentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores
	};

	if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to submit draw command buffer :(\n");
		return -1;
	}

	VkSwapchainKHR swapchains[] = {swapchain};
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapchains,
		.pImageIndices = &imageIndex,
		.pResults = NULL
	};

	result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
	if(result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapchain();
		return 0;
	} else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		fprintf(stderr, "Failed to present swapchain image\n");
		return -1;
	}

	currentFrame = (currentFrame + 1) % swapchainDetails.imageCount;

	return 0;
}

int recreateSwapchain(void)
{
	printf("Swapchain recreated\n");

	vkDeviceWaitIdle(device);

	cleanupSwapchain();

	//May need to add new renderpass if the image format changes
	createSwapchain();
	createDepthResources();
	createFramebuffers();

	return 0;
}

int processMainInput(void)
{
	SDL_Event event;

	while(SDL_PollEvent(&event) != 0)
	{
		//User requests quit
		if(event.type == SDL_EVENT_QUIT)
			return false;

		if(event.type == SDL_EVENT_WINDOW_RESIZED)
		{
			framebufferResized = true;
			int width = 0, height = 0;
			SDL_GetWindowSizeInPixels(window, &width, &height);
			while(width == 0 || height == 0)
			{
				SDL_GetWindowSizeInPixels(window, &width, &height);
				SDL_WaitEvent(&event);
			}

		}

		processInput(event);
	}

	return true;
}

int recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = 0,
		.pInheritanceInfo = NULL
	};

	if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to begin recording command buffer :(\n");
		return -1;
	}

	VkClearValue clearValues[] = {
		{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
		{.depthStencil = {1.0f, 0}}
	};

	VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = framebuffers[imageIndex],
		.renderArea.offset = {0, 0},
		.renderArea.extent = swapchainDetails.extent,
		.clearValueCount = 2,
		.pClearValues = clearValues
	};

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkBuffer vertexBuffers[] = {vertexBuffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	VkViewport viewport = {
		.x = 0.f,
		.y = 0.f,
		.width = (float)swapchainDetails.extent.width,
		.height = (float)swapchainDetails.extent.height,
		.minDepth = 0.f,
		.maxDepth = 1.f,
	};

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = swapchainDetails.extent,
	};

	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, NULL);

	vkCmdDrawIndexed(commandBuffer, sizeof(indices) / sizeof(uint16_t), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to record command buffer");
		return -1;
	}

	return 0;
}


int updateUniformBuffer(uint32_t currentImage)
{
	uint64_t time = SDL_GetTicks();

    ubo.resolution[0] = swapchainDetails.extent.width;
    ubo.resolution[1] = swapchainDetails.extent.height;
    ubo.time = (float)time/1000.f;

	memcpy(uniformBufferPtr[currentImage], &ubo, sizeof(ubo));

	return 0;
}

int cleanupSwapchain(void)
{
	vkDestroyImageView(device, depthImageView, NULL);
	vkDestroyImage(device, depthImage, NULL);
	vkFreeMemory(device, depthImageMemory, NULL);

	for(size_t i = 0; i < swapchainDetails.imageCount; i++)
	{
		vkDestroyFramebuffer(device, framebuffers[i], NULL);

	}

	for(size_t i = 0; i < swapchainDetails.imageCount; i++)
	{
		vkDestroyImageView(device, swapchainImageViews[i], NULL);
	}

	vkDestroySwapchainKHR(device, swapchain, NULL);

	return 0;
}
