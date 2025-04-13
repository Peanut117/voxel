#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <SDL3/SDL.h>

#include "define.h"
#include "vulkanGlobal.h"
#include "vulkan/vk_enum_string_helper.h"

#include "setup.h"

//HELP
//swapchainDetails.format = VK_FORMAT_B8G8R8A8_SRGB; //this line is wrong, ened to be fixed

/**************Structs****************/

/***********Vulkan Handles*************/
VkSwapchainKHR swapchain;

VkImageView* swapchainImageViews;

VkRenderPass renderPass;

VkFramebuffer* framebuffers;

VkDescriptorSetLayout descriptorSetLayout;
VkPipelineLayout graphicsPipelineLayout;

VkPipeline graphicsPipeline;

VkDescriptorPool descriptorPool;

VkDescriptorSet* descriptorSets;

VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;

VkBuffer* uniformBuffers;
VkDeviceMemory* uniformBuffersMemory;
void* uniformBufferPtr[3];

VkCommandPool commandPool;

VkCommandBuffer* commandBuffers;

VkSemaphore* imageAvailableSemaphores;
VkSemaphore* renderFinishedSemaphores;
VkFence* inFlightFences;

VkBuffer vertexBuffer = VK_NULL_HANDLE;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

/**************Variables****************/
swapchainImageDetails swapchainDetails;

/************Function Declarations**********/
int createSwapchain(void);
int createRenderPass(void);
int createDescriptorSetLayout(void);
int createGraphicspipeline(void);
int createDescriptorPool(void);
int createUniformBuffers(void);
int createDescriptorSets(void);
int createCommandPool(void);
int createCommandBuffers(void);
int createVertexBuffer(void);
int createIndexBuffer(void);
int createDepthResources(void);
int createFramebuffers(void);
int createSyncObjects(void);

/************Helper Function Declarations**********/
swapchainSupportDetails gatherSwapchainSupportDetails(VkPhysicalDevice device); //Global
void cleanupSwapchainSupportDetails(swapchainSupportDetails details); //Global heaps i hope
VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR* surfaceFormats, uint32_t surfaceFormatCount);
VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities);
VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* presentModes, uint32_t presentModeCount);
VkShaderModule createShaderModule(const char* fileName);
VkVertexInputAttributeDescription* getVertexAttributeDescriptions(uint32_t* attributeCount);
int createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* pBuffer, VkDeviceMemory* pBufferMemory);
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
int copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
VkFormat findDepthFormat(void);
int createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage* image, VkDeviceMemory* imageMemory);
VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

/***********Create Global Functions***********/
int setupVulkan(void)
{
	createSwapchain();
	createDepthResources();
	createRenderPass();
	createFramebuffers();
	createDescriptorSetLayout();
	createDescriptorPool();
	createUniformBuffers();
	createDescriptorSets();
	createGraphicspipeline();
	createCommandPool();
	createCommandBuffers();
	createVertexBuffer();
	createIndexBuffer();
	createSyncObjects();

	return 0;
}

void cleanupSetupVulkan(void)
{
	vkDeviceWaitIdle(device);

	cleanupSwapchain();

	for(uint32_t i = 0; i < swapchainDetails.imageCount; i++)
	{
		vkDestroyBuffer(device, uniformBuffers[i], NULL);
		vkFreeMemory(device, uniformBuffersMemory[i], NULL);
	}

	vkDestroyDescriptorPool(device, descriptorPool, NULL);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);

	vkDestroyBuffer(device, vertexBuffer, NULL);
	vkFreeMemory(device, vertexBufferMemory, NULL);

	vkDestroyBuffer(device, indexBuffer, NULL);
	vkFreeMemory(device, indexBufferMemory, NULL);

	vkDestroyPipeline(device, graphicsPipeline, NULL);
	vkDestroyPipelineLayout(device, graphicsPipelineLayout, NULL);

	vkDestroyRenderPass(device, renderPass, NULL);

	for(uint32_t i = 0; i < swapchainDetails.imageCount; i++)
	{
		vkDestroySemaphore(device, imageAvailableSemaphores[i], NULL);
		vkDestroySemaphore(device, renderFinishedSemaphores[i], NULL);
		vkDestroyFence(device, inFlightFences[i], NULL);
	}

	vkDestroyCommandPool(device, commandPool, NULL);
}

/***********Create Functions***********/

int createSwapchain(void)
{
	swapchainSupportDetails swapchainSupport = gatherSwapchainSupportDetails(physicalDevice);

	VkSurfaceFormatKHR imageFormat = chooseSwapSurfaceFormat(swapchainSupport.surfaceFormats, swapchainSupport.surfaceFormatCount);
	VkExtent2D imageExtent = chooseSwapExtent(swapchainSupport.surfaceCapabilities);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes, swapchainSupport.presentModeCount);

	uint32_t imageCount = (!swapchainSupport.surfaceCapabilities.maxImageCount && swapchainSupport.surfaceCapabilities.minImageCount+1 > swapchainSupport.surfaceCapabilities.maxImageCount) ?
				swapchainSupport.surfaceCapabilities.minImageCount + 1 :
				swapchainSupport.surfaceCapabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = imageFormat.format,
		.imageColorSpace = imageFormat.colorSpace,
		.imageExtent = imageExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.preTransform = swapchainSupport.surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain));

	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
	VkImage swapchainImages[swapchainImageCount];
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages);

	swapchainImageViews = malloc(swapchainImageCount * sizeof(VkImageView));

	for(uint32_t i = 0; i < swapchainImageCount; i++)
	{
		VkImageViewCreateInfo imageViewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.image = swapchainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = imageFormat.format,
			.components.r = VK_COMPONENT_SWIZZLE_R,
			.components.g = VK_COMPONENT_SWIZZLE_G,
			.components.b = VK_COMPONENT_SWIZZLE_B,
			.components.a = VK_COMPONENT_SWIZZLE_A,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1
		};

		VK_CHECK(vkCreateImageView(device, &imageViewInfo, NULL, &swapchainImageViews[i]));
	}

	swapchainDetails.imageCount = swapchainImageCount;
	swapchainDetails.format = imageFormat.format;
	swapchainDetails.extent = imageExtent;

	//cleanupSwapchainSupportDetails(swapchainSupport);

	return 0;
}

int createRenderPass(void)
{
	VkAttachmentDescription colorAttachment = {
		.flags = 0,
		.format = swapchainDetails.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription depthAttachment = {
		.flags = 0,
		.format = findDepthFormat(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = &depthAttachmentRef,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL,
	};

	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	};


	VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};

	VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass));

	return 0;
}

int createFramebuffers(void)
{
	framebuffers = malloc(swapchainDetails.imageCount * sizeof(VkFramebuffer));

	for(uint32_t i = 0; i < swapchainDetails.imageCount; i++)
	{
		VkImageView attachments[] = {swapchainImageViews[i], depthImageView};

		VkFramebufferCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.renderPass = renderPass,
			.attachmentCount = 2,
			.pAttachments = attachments,
			.width = swapchainDetails.extent.width,
			.height = swapchainDetails.extent.height,
			.layers = 1
		};

		VK_CHECK(vkCreateFramebuffer(device, &createInfo, NULL, &framebuffers[i]));
	}

	return 0;
}

int createDescriptorSetLayout(void)
{
	VkDescriptorSetLayoutBinding layoutBinding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = NULL
	};

	VkDescriptorSetLayoutCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = 1,
		.pBindings = &layoutBinding
	};

	VK_CHECK(vkCreateDescriptorSetLayout(device, &createInfo, NULL, &descriptorSetLayout));

	VkPipelineLayoutCreateInfo pipelineInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL
	};

	VK_CHECK(vkCreatePipelineLayout(device, &pipelineInfo, NULL, &graphicsPipelineLayout));

	return 0;
}

int createGraphicspipeline(void)
{
	VkShaderModule vertModule = createShaderModule("shaders/vert.spv");
	VkShaderModule fragModule = createShaderModule("shaders/frag.spv");

	const uint32_t shaderStages = 2;
	VkPipelineShaderStageCreateInfo shaderInfo[shaderStages] = {
		[0] = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertModule,
			.pName = "main",
			.pSpecializationInfo = NULL
		},
		[1] = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragModule,
			.pName = "main",
			.pSpecializationInfo = NULL
		}
	};

	VkVertexInputBindingDescription bindingDescription = {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	uint32_t attributeCount;
	VkVertexInputAttributeDescription* attrDescription = getVertexAttributeDescriptions(&attributeCount);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = attributeCount,
		.pVertexAttributeDescriptions = attrDescription
	};

	VkPipelineInputAssemblyStateCreateInfo assemblyStateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkPipelineViewportStateCreateInfo viewportInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = NULL,
		.scissorCount = 1,
		.pScissors = NULL
	};

	VkPipelineRasterizationStateCreateInfo rasterizerInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f
	};

	VkPipelineMultisampleStateCreateInfo multisampleInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE
	};

	VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment
	};

	const uint32_t dynamicStateCount = 2;
	VkDynamicState dynamicStates[dynamicStateCount] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = dynamicStateCount,
		.pDynamicStates = dynamicStates
	};

	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stageCount = shaderStages,
		.pStages = shaderInfo,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &assemblyStateInfo,
		.pTessellationState = NULL,
		.pViewportState = &viewportInfo,
		.pRasterizationState = &rasterizerInfo,
		.pMultisampleState = &multisampleInfo,
		.pDepthStencilState = &depthStencilInfo,
		.pColorBlendState = &colorBlendInfo,
		.pDynamicState = &dynamicStateInfo,
		.layout = graphicsPipelineLayout,
		.renderPass = renderPass,
		.subpass = 0
	};

	VK_CHECK(vkCreateGraphicsPipelines(device, NULL, 1, &pipelineInfo, NULL, &graphicsPipeline));

	vkDestroyShaderModule(device, vertModule, NULL);
	vkDestroyShaderModule(device, fragModule, NULL);

	return 0;
}

int createDescriptorPool(void)
{
	const uint32_t poolSizeCount = 1;
	VkDescriptorPoolSize poolSizes[poolSizeCount] = {
	{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = swapchainDetails.imageCount
		}
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = swapchainDetails.imageCount,
		.poolSizeCount = poolSizeCount,
		.pPoolSizes = poolSizes
	};

	VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolInfo, NULL, &descriptorPool));

	return 0;
}

int createUniformBuffers(void)
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	uniformBuffers = malloc(swapchainDetails.imageCount * sizeof(VkBuffer));
	uniformBuffersMemory = malloc(swapchainDetails.imageCount * sizeof(VkDeviceMemory));
	//uniformBufferPtr = (void*)malloc(swapchainDetails.imageCount * sizeof(void*));

	for(uint32_t i = 0; i < swapchainDetails.imageCount; i++)
	{
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffers[i], &uniformBuffersMemory[i]);

		VK_CHECK(vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBufferPtr[i]));
	}

	return 0;
}

int createDescriptorSets(void)
{
	VkDescriptorSetLayout layouts[swapchainDetails.imageCount];

	for(uint32_t i = 0; i < swapchainDetails.imageCount; i++)
		layouts[i] = descriptorSetLayout;

	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = swapchainDetails.imageCount,
		.pSetLayouts = layouts
	};

	descriptorSets = malloc(swapchainDetails.imageCount * sizeof(VkDescriptorSet));

	VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets));

	for(uint32_t i = 0; i < swapchainDetails.imageCount; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {
			.buffer = uniformBuffers[i],
			.offset = 0,
			.range = sizeof(UniformBufferObject)
		};

		VkWriteDescriptorSet descriptorWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.pBufferInfo = &bufferInfo,
			.pImageInfo = NULL,
			.pTexelBufferView = NULL
		};

		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);
	}

	return 0;
}

/*int createVertexBuffer(void)
{
	VkDeviceSize bufferSize = vertexCount * sizeof(Vertex);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices, (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);

	return 0;
}*/
int createVertexBuffer(void) {
    VkDeviceSize bufferSize = sizeof(vertices);

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    if (createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &stagingBuffer, &stagingBufferMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create staging buffer!\n");
        return -1;
    }

    // Map and copy data
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create vertex buffer (device-local)
    if (createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    &vertexBuffer, &vertexBufferMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vertex buffer!\n");
        vkDestroyBuffer(device, stagingBuffer, NULL);
        vkFreeMemory(device, stagingBufferMemory, NULL);
        return -1;
    }

    // Copy staging → vertex buffer
    if (copyBuffer(stagingBuffer, vertexBuffer, bufferSize) != 0) {
        fprintf(stderr, "Failed to copy vertex buffer!\n");
        vkDestroyBuffer(device, vertexBuffer, NULL);
        vkFreeMemory(device, vertexBufferMemory, NULL);
        vkDestroyBuffer(device, stagingBuffer, NULL);
        vkFreeMemory(device, stagingBufferMemory, NULL);
        return -1;
    }

    // Cleanup staging resources
    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);

    return 0;
}

int createIndexBuffer(void)
{
	VkDeviceSize bufferSize = sizeof(indices);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices, (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);
	
	return 0;
}

int createCommandPool(void)
{
	QueueFamilyIndices indices = gatherQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = indices.graphicsFamilyIndex
	};

	VK_CHECK(vkCreateCommandPool(device, &createInfo, NULL, &commandPool));

	return 0;
}

int createCommandBuffers(void)
{
	commandBuffers = malloc(swapchainDetails.imageCount * sizeof(VkCommandBuffer));

	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = swapchainDetails.imageCount
	};

	VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers));

	return 0;
}

int createSyncObjects(void)
{
	imageAvailableSemaphores = malloc(swapchainDetails.imageCount * sizeof(VkSemaphore));
	renderFinishedSemaphores = malloc(swapchainDetails.imageCount * sizeof(VkSemaphore));
	inFlightFences = malloc(swapchainDetails.imageCount * sizeof(VkFence));

	VkSemaphoreCreateInfo semaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	VkFenceCreateInfo fenceInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for(size_t i = 0; i < swapchainDetails.imageCount; i++)
	{
		if(vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphores[i]) != VK_SUCCESS
				|| vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphores[i]) != VK_SUCCESS
				|| vkCreateFence(device, &fenceInfo, NULL, &inFlightFences[i]) != VK_SUCCESS)
		{
			fprintf(stderr, "Failed to create sync objects :(\n");
			return -1;
		}
	}

	return 0;
}

int createDepthResources(void)
{
	VkFormat depthFormat = findDepthFormat(); // Ensure this is supported
    createImage(swapchainDetails.extent.width, swapchainDetails.extent.height, depthFormat, 
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthImage, &depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	return 0;
}

swapchainSupportDetails gatherSwapchainSupportDetails(VkPhysicalDevice device)
{
	swapchainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.surfaceCapabilities);

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, NULL);

	details.presentModes = malloc(details.presentModeCount * sizeof(VkPresentModeKHR));
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, details.presentModes);

	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.surfaceFormatCount, NULL);

	details.surfaceFormats = malloc(details.surfaceFormatCount * sizeof(VkSurfaceFormatKHR));
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.surfaceFormatCount, details.surfaceFormats);

	return details;
}

void cleanupSwapchainSupportDetails(swapchainSupportDetails details)
{
	free(details.presentModes);
	free(details.surfaceFormats);
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR* surfaceFormats, uint32_t surfaceFormatCount)
{
	for(uint32_t i = 0; i < surfaceFormatCount; i++)
	{
		if(surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return surfaceFormats[i];
		}
	}

	return (VkSurfaceFormatKHR){0};
}

VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities)
{
	if(capabilities.currentExtent.width != UINT_MAX)
		return capabilities.currentExtent;

	int width, height;
	SDL_GetWindowSizeInPixels(window, &width, &height);

	VkExtent2D extent = {
		.width = (uint32_t)width,
		.height = (uint32_t)height
	};

	//Clamp width and height between the allowed extents that are supported
	extent.width = extent.width < capabilities.minImageExtent.width ? capabilities.minImageExtent.width : extent.width;
	extent.width = extent.width > capabilities.maxImageExtent.width ? capabilities.maxImageExtent.width : extent.width;

	extent.height = extent.height < capabilities.minImageExtent.height ? capabilities.minImageExtent.height : extent.height;
	extent.height = extent.height > capabilities.maxImageExtent.height ? capabilities.maxImageExtent.height : extent.height;

	return extent;
}

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* presentModes, uint32_t presentModeCount)
{
	for(uint32_t i = 0; i < presentModeCount; i++)
	{
		if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			return presentModes[i];
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkShaderModule createShaderModule(const char* fileName)
{
    int fd = open(fileName, O_RDONLY);
	struct stat sb;

	if(fd == -1)
	{
		perror("No file :(\n");
		return NULL;
	}

	if(fstat(fd, &sb) == -1)
	{
		perror("Failed to fstat file\n");
		return NULL;
	}

	size_t size = sb.st_size;

	char* code = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	//close(fd);

	uint32_t* codeConverted = (uint32_t*)code;

	VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = codeConverted
	};

	VkShaderModule shaderModule;
	if(vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to create shader module: %s\n", fileName);
	}

	return shaderModule;
}

VkVertexInputAttributeDescription* getVertexAttributeDescriptions(uint32_t* attributeCount)
{
    // Define the number of attributes (e.g., position and color)
    *attributeCount = 2;

    // Allocate memory for the attribute descriptions
    VkVertexInputAttributeDescription* attributeDescriptions = malloc(*attributeCount * sizeof(VkVertexInputAttributeDescription));

    // Position attribute
    attributeDescriptions[0] = (VkVertexInputAttributeDescription) {
        .location = 0, // Location in the vertex shader
        .binding = 0, // Binding index (matches the binding description)
        .format = VK_FORMAT_R32G32B32_SFLOAT, // Format of the position data (vec3)
        .offset = offsetof(Vertex, pos) // Offset of the position data in the Vertex struct
    };

    // Color attribute
    attributeDescriptions[1] = (VkVertexInputAttributeDescription) {
        .location = 1, // Location in the vertex shader
        .binding = 0, // Binding index (matches the binding description)
        .format = VK_FORMAT_R32G32B32_SFLOAT, // Format of the color data (vec3)
        .offset = offsetof(Vertex, color) // Offset of the color data in the Vertex struct
    };

    return attributeDescriptions;
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	fprintf(stderr, "Failed to find suitable memory type :(\n");
	exit(EXIT_FAILURE);
}

int createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* pBuffer, VkDeviceMemory* pBufferMemory)
{
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkResult result = vkCreateBuffer(device, &bufferInfo, NULL, pBuffer);
	VK_CHECK(result);

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, *pBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
	};

	result = vkAllocateMemory(device, &allocInfo, NULL, pBufferMemory);
	VK_CHECK(result);

	VK_CHECK(vkBindBufferMemory(device, *pBuffer, *pBufferMemory, 0));

	return 0;
}

int copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = commandPool,
		.commandBufferCount = 1
	};


	VkCommandBuffer commandBuffer;
	VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

	return 0;
}

VkFormat findSupportedFormat(const VkFormat candidates[], uint32_t candidateCount, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for(uint32_t i = 0; i < candidateCount; i++)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, candidates[i], &props);

		if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return candidates[i];
		} else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return candidates[i];
		}
	}

	fprintf(stderr, "Format undefined\n");
	return VK_FORMAT_UNDEFINED;
}

VkFormat findDepthFormat(void)
{
	return findSupportedFormat(
			(VkFormat[]){VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
			3,
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);
}

int createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage* image, VkDeviceMemory* imageMemory)
{
	VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = width,
		.extent.height = height,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = format,
		.tiling = tiling,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = usage,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

    if (vkCreateImage(device, &imageInfo, NULL, image) != VK_SUCCESS) {
        fprintf(stderr, "failed to create image!\n");
		return -1;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, *image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties),
	};

    if (vkAllocateMemory(device, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
        fprintf(stderr, "failed to allocate image memory!\n");
		return -1;
    }

    vkBindImageMemory(device, *image, *imageMemory, 0);

	return 0;
}

VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange.aspectMask = aspectFlags,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
	};

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, NULL, &imageView) != VK_SUCCESS) {
        fprintf(stderr, "failed to create image view\n");
    }

    return imageView;
}
