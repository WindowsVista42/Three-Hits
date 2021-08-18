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
    u32 vertex_count;
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

typedef struct PipelineOptions {
    VkCullModeFlags cull_mode;
    VkFrontFace front_face;

    VkAttachmentLoadOp color_load_op;
    VkAttachmentStoreOp color_store_op;
    VkImageLayout color_initial_layout;
    VkImageLayout color_final_layout;

    VkAttachmentLoadOp depth_load_op;
    VkAttachmentStoreOp depth_store_op;
    VkImageLayout depth_initial_layout;
    VkImageLayout depth_final_layout;
} PipelineOptions;

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
    Buffer* local_buffer,
    Buffer* staging_buffer
) {
    create_buffer(
        device,
        physical_device,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buffer->buffer,
        &staging_buffer->memory
    );

    write_buffer(device, staging_buffer->memory, 0, size, 0, data);

    create_buffer(
        device,
        physical_device,
        size,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &local_buffer->buffer,
        &local_buffer->memory
    );

    copy_buffer_to_buffer(device, queue, command_pool, staging_buffer->buffer, local_buffer->buffer, size);
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
    sampler_create_info.magFilter = VK_FILTER_NEAREST;
    sampler_create_info.minFilter = VK_FILTER_NEAREST;
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

void create_graphics_pipeline(
    VkDevice device,
    VkShaderModule vertex_module,
    VkShaderModule fragment_module,
    u32 vertex_attribute_description_count,
    const VkVertexInputAttributeDescription * vertex_attribute_descriptions,
    u32 vertex_binding_description_count,
    const VkVertexInputBindingDescription* vertex_binding_descriptions,
    u32 swapchain_width,
    u32 swapchain_height,
    u32 descriptor_set_layout_count,
    VkDescriptorSetLayout* descriptor_set_layout,
    PipelineOptions* pipeline_options,
    Pipeline* pipeline,
    VkFormat swapchain_format,
    VkFormat depth_image_format
) {
    // TODO(sean): figure out if this need to be used for anything
    // VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };

    // VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    // dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // dynamic_state_create_info.dynamicStateCount = 2;
    // dynamic_state_create_info.pDynamicStates = dynamic_states;

    VkPipelineShaderStageCreateInfo vertex_stage_create_info = {};
    vertex_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_stage_create_info.module = vertex_module;
    vertex_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_stage_create_info = {};
    fragment_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_stage_create_info.module = fragment_module;
    fragment_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_stage_create_info, fragment_stage_create_info };

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_attribute_description_count;
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions;
    vertex_input_state_create_info.vertexBindingDescriptionCount = vertex_binding_description_count;
    vertex_input_state_create_info.pVertexBindingDescriptions = vertex_binding_descriptions;

    VkPipelineInputAssemblyStateCreateInfo vertex_input_assembly_create_info = {};
    vertex_input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    vertex_input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    vertex_input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)swapchain_width;
    viewport.height = (f32)swapchain_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = swapchain_width;
    scissor.extent.height = swapchain_height;

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.lineWidth = 1.0f;
    rasterization_state_create_info.cullMode = pipeline_options->cull_mode;
    rasterization_state_create_info.frontFace = pipeline_options->front_face;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    rasterization_state_create_info.depthBiasClamp = 0.0f;
    rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.minSampleShading = 1.0f;
    multisample_state_create_info.pSampleMask = 0;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.blendEnable = VK_TRUE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0] = 0.0f;
    color_blend_state_create_info.blendConstants[1] = 0.0f;
    color_blend_state_create_info.blendConstants[2] = 0.0f;
    color_blend_state_create_info.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = descriptor_set_layout_count;
    pipeline_layout_create_info.pSetLayouts = descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = 0;

    if(vkCreatePipelineLayout(device, &pipeline_layout_create_info, 0, &pipeline->layout) != VK_SUCCESS) {
        panic("Failed to create pipeline layout!");
    }

    VkAttachmentDescription color_attachment_desc = {};
    color_attachment_desc.format = swapchain_format;
    color_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_desc.loadOp = pipeline_options->color_load_op;
    color_attachment_desc.storeOp = pipeline_options->color_store_op;
    color_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_desc.initialLayout = pipeline_options->color_initial_layout;
    color_attachment_desc.finalLayout = pipeline_options->color_final_layout;

    VkAttachmentDescription depth_attachment_desc = {};
    depth_attachment_desc.format = depth_image_format;
    depth_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment_desc.loadOp = pipeline_options->depth_load_op;
    depth_attachment_desc.storeOp = pipeline_options->depth_store_op;
    depth_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_desc.initialLayout = pipeline_options->depth_initial_layout;
    depth_attachment_desc.finalLayout = pipeline_options->depth_final_layout;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc[1] = {};
    subpass_desc[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc[0].colorAttachmentCount = 1;
    subpass_desc[0].pColorAttachments = &color_attachment_ref;
    subpass_desc[0].pDepthStencilAttachment = &depth_attachment_ref;
//    subpass_desc[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//    subpass_desc[1].colorAttachmentCount = 1;
//    subpass_desc[1].pColorAttachments = &color_attachment_ref;
//    subpass_desc[1].pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {color_attachment_desc, depth_attachment_desc};
    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 2;
    render_pass_create_info.pAttachments = attachments;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = subpass_desc;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;

    if(vkCreateRenderPass(device, &render_pass_create_info, 0, &pipeline->pass) != VK_SUCCESS) {
        //TODO(sean): diagnostic
        panic("Failed to create render pass!");
    }

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.minDepthBounds = 0.0f;
    depth_stencil_state_create_info.maxDepthBounds = 1.0f;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    //depth_stencil_state_create_info.front = ;
    //depth_stencil_state_create_info.back = ;

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &vertex_input_assembly_create_info;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState = 0;
    pipeline_create_info.layout = pipeline->layout;
    pipeline_create_info.renderPass = pipeline->pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = 0;
    pipeline_create_info.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(device, 0, 1, &pipeline_create_info, 0, &pipeline->pipeline) != VK_SUCCESS) {
        //TODO(sean): diagnostic
        panic("Failed to create graphics pipeline!");
    }
}

void destroy_pipeline(VkDevice device, Pipeline pipeline) {
    vkDestroyPipeline(device, pipeline.pipeline, 0);
    vkDestroyPipelineLayout(device, pipeline.layout, 0);
    vkDestroyRenderPass(device, pipeline.pass, 0);
}

void destroy_texture(VkDevice device, Texture texture) {
    vkDestroyImageView(device, texture.view, 0);
    vkDestroyImage(device, texture.image, 0);
    vkFreeMemory(device, texture.memory, 0);
}

#define UNTITLED_FPS_VKUTIL_H
#endif //UNTITLED_FPS_VKUTIL_H
