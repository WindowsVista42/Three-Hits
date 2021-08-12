//
// Created by Sean Moulton on 7/24/2021.
//

#ifndef UNTITLED_FPS_VKUTIL_H

#include <vulkan/vulkan.h>
#include "util.h"
#include "stdio.h"

typedef struct Buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
} Buffer;

typedef struct Model {
    Buffer vertices;
    u32 index_count;
    Buffer indices;
} Model;

typedef struct Texture {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
} Texture;

typedef struct Pipeline {
    VkPipelineLayout layout;
    VkRenderPass pass;
    VkPipeline pipeline;
} Pipeline;

typedef struct Modules {
    VkShaderModule vertex;
    VkShaderModule fragment;
} Modules;

global VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data
) {
    char message_type_str[14];
    char message_severity_str[10];
    char output_str[512];

    switch(message_type) {
        case(VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT): {
            sprintf(message_type_str, "[PERFORMANCE]");
        } break;
        case(VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT): {
            sprintf(message_type_str, "[VALIDATION]");
        } break;
        case(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT): {
            sprintf(message_type_str, "[GENERAL]");
        } break;
        default: {
            sprintf(message_type_str, "[UNKNOWN]");
        } break;
    }

    switch(message_severity) {
        case(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT): {
            sprintf(message_severity_str, "[INFO]");
        } break;
        case(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT): {
            sprintf(message_severity_str, "[ERROR]");
        } break;
        case(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT): {
            sprintf(message_severity_str, "[WARNING]");
        } break;
        case(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT): {
            sprintf(message_severity_str, "[VERBOSE]");
        } break;
        default: {
            sprintf(message_severity_str, "[UNKNOWN]");
        } break;
    }

    sprintf(output_str, "%s %s %s", message_type_str, message_severity_str, p_callback_data->pMessage);
    fprintf(stderr, "%s\n\n", output_str);

    return VK_FALSE;
}

global inline void write_buffer(
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void* data
) {
    void* ptr;
    vkMapMemory(device, memory, offset, size, flags, &ptr);
        memcpy(ptr, data, size);
    vkUnmapMemory(device, memory);
}

void create_shader_module(VkDevice device, const char* buffer, usize buffer_size, VkShaderModule* shader_module) {
    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = buffer_size;
    shader_module_create_info.pCode = (u32*)buffer;
    if(vkCreateShaderModule(device, &shader_module_create_info, 0, shader_module) != VK_SUCCESS) {
        panic("Failed to create shader module!");
    }
}

u32 find_memory_type(VkPhysicalDevice physical_device, u32 type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for(u32 index = 0; index < memory_properties.memoryTypeCount; index += 1) {
        if((type_filter & (1 << index)) && (memory_properties.memoryTypes[index].propertyFlags & properties) == properties) {
            return index;
        }
    }

    panic("Failed to find suitable memory type!");
}

VkCommandBuffer begin_quick_commands(
    VkDevice device,
    VkCommandPool command_pool
) {
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = command_pool;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = 0;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
}

void end_quick_commands(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkCommandBuffer command_buffer) {
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(queue, 1, &submit_info, 0);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void create_buffer(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer* buffer,
    VkDeviceMemory* buffer_memory
) {
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(device, &buffer_create_info, 0, buffer) != VK_SUCCESS) {
        panic("Failed to create vertex buffer!");
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memory_requirements);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = find_memory_type(physical_device, memory_requirements.memoryTypeBits, properties);

    if(vkAllocateMemory(device, &allocate_info, 0, buffer_memory) != VK_SUCCESS) {
        panic("Failed to allocate vertex buffer!");
    }

    vkBindBufferMemory(device, *buffer, *buffer_memory, 0);
}

void transition_image_layout(
    VkDevice device,
    VkQueue queue,
    VkCommandPool command_pool,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout
) {
    VkCommandBuffer command_buffer = begin_quick_commands(device, command_pool);
    {
        VkPipelineStageFlags src_stage_flags, dst_stage_flags;

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            src_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;

        } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            src_stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dst_stage_flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        } else {
            panic("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            command_buffer,
            src_stage_flags, dst_stage_flags,
            0,
            0, 0,
            0, 0,
            1, &barrier
        );
    }
    end_quick_commands(device, queue, command_pool, command_buffer);
}

void copy_buffer_to_image(
    VkDevice device,
    VkQueue queue,
    VkCommandPool command_pool,
    VkBuffer buffer,
    VkImage image,
    u32 width,
    u32 height
) {
    VkCommandBuffer command_buffer = begin_quick_commands(device, command_pool);
    {
        VkBufferImageCopy image_copy = {};
        image_copy.bufferOffset = 0;
        image_copy.bufferRowLength = 0;
        image_copy.bufferImageHeight = 0;
        image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_copy.imageSubresource.mipLevel = 0;
        image_copy.imageSubresource.baseArrayLayer = 0;
        image_copy.imageSubresource.layerCount = 1;
        image_copy.imageOffset.x = 0;
        image_copy.imageOffset.y = 0;
        image_copy.imageOffset.z = 0;
        image_copy.imageExtent.width = width;
        image_copy.imageExtent.height = height;
        image_copy.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
    }
    end_quick_commands(device, queue, command_pool, command_buffer);
}

void create_image(
    VkDevice device,
    VkPhysicalDevice physical_device,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage* image,
    VkDeviceMemory* image_memory
) {
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.flags = 0;

    if(vkCreateImage(device, &image_create_info, 0, image) != VK_SUCCESS) {
        panic("Failed to create image!");
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, *image, &memory_requirements);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = find_memory_type(physical_device, memory_requirements.memoryTypeBits, properties);

    if(vkAllocateMemory(device, &allocate_info, 0, image_memory) != VK_SUCCESS) {
        panic("Failed to allocate image memory!");
    }

    vkBindImageMemory(device, *image, *image_memory, 0);
}

void copy_buffer_to_buffer(
    VkDevice device,
    VkQueue queue,
    VkCommandPool command_pool,
    VkBuffer src,
    VkBuffer dst,
    VkDeviceSize size
) {
    VkCommandBuffer command_buffer = begin_quick_commands(device, command_pool);
    {
        VkBufferCopy copy_region = {};
        copy_region.srcOffset = 0;
        copy_region.dstOffset = 0;
        copy_region.size = size;

        vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);
    }

    end_quick_commands(device, queue, command_pool, command_buffer);
}

void create_device_local_buffer(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkQueue queue,
    VkCommandPool command_pool,
    VkDeviceSize size,
    void* data,
    VkBufferUsageFlags usage,
    VkBuffer* local_buffer,
    VkDeviceMemory* local_memory
) {
    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    create_buffer(device, physical_device, size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &staging_buffer, &staging_memory);

    write_buffer(device, staging_memory, 0, size, 0, data);

    create_buffer(device, physical_device, size,
                  usage,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  local_buffer, local_memory);

    copy_buffer_to_buffer(device, queue, command_pool, staging_buffer, *local_buffer, size);

    vkDestroyBuffer(device, staging_buffer, 0);
    vkFreeMemory(device, staging_memory, 0);
}

void create_device_local_and_staging_buffer(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkQueue queue,
    VkCommandPool command_pool,
    VkDeviceSize size,
    void* data,
    VkBufferUsageFlags usage,
    VkBuffer* local_buffer,
    VkDeviceMemory* local_memory,
    VkBuffer* staging_buffer,
    VkDeviceMemory* staging_memory
) {
    create_buffer(device, physical_device, size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_buffer, staging_memory);

    write_buffer(device, *staging_memory, 0, size, 0, data);

    create_buffer(device, physical_device, size,
                  usage,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  local_buffer, local_memory);

    copy_buffer_to_buffer(device, queue, command_pool, *staging_buffer, *local_buffer, size);
}

void create_device_local_image(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkQueue queue,
    VkCommandPool command_pool,
    u32 width,
    u32 height,
    VkDeviceSize size,
    void* data,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImage* texture_image,
    VkDeviceMemory* texture_image_memory
) {
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    create_buffer(
        device,
        physical_device,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buffer,
        &staging_buffer_memory
    );

    write_buffer(device, staging_buffer_memory, 0, size, 0, data);

    create_image(
        device,
        physical_device,
        width,
        height,
        format,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        texture_image,
        texture_image_memory
    );

    transition_image_layout(
        device,
        queue,
        command_pool,
        *texture_image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    copy_buffer_to_image(device, queue, command_pool, staging_buffer, *texture_image, width, height);
    transition_image_layout(
        device,
        queue,
        command_pool,
        *texture_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    vkDestroyBuffer(device, staging_buffer, 0);
    vkFreeMemory(device, staging_buffer_memory, 0);
}

void create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, VkImageView* image_view) {
    VkImageViewCreateInfo view_create_info = {};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = format;
    view_create_info.subresourceRange.aspectMask = aspect;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &view_create_info, 0, image_view) != VK_SUCCESS) {
        panic("Failed to create texture image view!");
    }
}

void create_image_sampler(VkDevice device, VkPhysicalDevice physical_device, VkSampler* sampler) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    sampler_create_info.compareEnable = VK_TRUE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;

    if(vkCreateSampler(device, &sampler_create_info, 0, sampler) != VK_SUCCESS) {
        panic("Failed to create sampler!");
    }
}

VkFormat find_supported_format(VkPhysicalDevice physical_device, u32 format_count, VkFormat* formats, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for(u32 index = 0; index < format_count; index += 1) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physical_device, formats[index], &properties);

        if(tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
            return formats[index];
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
            return formats[index];
        }
    }

    panic("Failed to find supported format!");
}

#define UNTITLED_FPS_VKUTIL_H
#endif //UNTITLED_FPS_VKUTIL_H
