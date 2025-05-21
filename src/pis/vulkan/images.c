#include "images.h"
#include "initializers.h"
#include "pisdef.h"
#include "vulkan/vulkan_core.h"
#include "buffers.h"

void CreateImage(VkDevice device,
                 VkPhysicalDevice pDevice,
                 VkFormat format,
                 VkImageType imageType,
                 VkImageUsageFlags imageUsage,
                 VkExtent3D extent,
                 VkMemoryPropertyFlags properties,
                 VkImage* image,
                 VkDeviceMemory* imageMemory)
{
    VkImageCreateInfo drawImgInfo = ImageCreateInfo(format, imageType, imageUsage, extent);
    VK_CHECK(vkCreateImage(device, &drawImgInfo, NULL, image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, *image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = FindMemoryType(pDevice, memRequirements.memoryTypeBits, properties),
    };

    if (vkAllocateMemory(device, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
        fprintf(stderr, "failed to allocate image memory!\n");
        exit(-1);
    }

    vkBindImageMemory(device, *image, *imageMemory, 0);
}

void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkPipelineStageFlags srcStage = 0;
    VkPipelineStageFlags dstStage = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else {
        // Add other transitions as needed
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }

    vkCmdPipelineBarrier(
        cmd,
        srcStage,  // srcStageMask
        dstStage,  // dstStageMask
        0,
        0, NULL,                             // memory barriers
        0, NULL,                             // buffer barriers
        1, &barrier                          // image barriers
    );
}

void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize)
{
    VkImageBlit blitRegion = {
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .srcOffsets = {
            {0, 0, 0},
            { (int32_t)srcSize.width, (int32_t)srcSize.height, 1 }
        },
        .dstOffsets = {
            {0, 0, 0},
            { (int32_t)dstSize.width, (int32_t)dstSize.height, 1 }
        }
    };

    vkCmdBlitImage(
        cmd,
        source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blitRegion,
        VK_FILTER_LINEAR
    );
}
