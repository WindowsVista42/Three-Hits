//
// Created by Windows Vista on 7/24/2021.
//

#ifndef UNTITLED_FPS_VKUTIL_H

#include <vulkan/vulkan.h>
#include "util.h"
#include "stdio.h"

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
    return UINT32_MAX;
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

void copy_buffer_to_buffer(
    VkQueue queue,
    VkDevice device,
    VkCommandPool command_pool,
    VkBuffer src,
    VkBuffer dst,
    VkDeviceSize size
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

        VkBufferCopy copy_region = {};
        copy_region.srcOffset = 0;
        copy_region.dstOffset = 0;
        copy_region.size = size;

        vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(queue, 1, &submit_info, 0);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void create_device_local_buffer(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkQueue queue,
    VkCommandPool command_pool,
    VkDeviceSize size,
    void* data,
    VkBufferUsageFlags usage,
    VkBuffer* buffer,
    VkDeviceMemory* buffer_memory
) {
    //TODO(sean): see if we want to put this to a debug flag or not
    assert(VK_BUFFER_USAGE_TRANSFER_DST_BIT & usage);

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(device, physical_device, size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &staging_buffer, &staging_buffer_memory);

    write_buffer(device, staging_buffer_memory, 0, size, 0, data);

    create_buffer(device, physical_device, size,
                  usage,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  buffer, buffer_memory);

    copy_buffer_to_buffer(queue, device, command_pool, staging_buffer, *buffer, size);

    vkDestroyBuffer(device, staging_buffer, 0);
    vkFreeMemory(device, staging_buffer_memory, 0);
}

#define UNTITLED_FPS_VKUTIL_H
#endif //UNTITLED_FPS_VKHELPERS_H
