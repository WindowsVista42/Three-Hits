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

void create_shader_module(VkDevice device, const char* buffer, usize buffer_size, VkShaderModule* shader_module) {
    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = buffer_size;
    shader_module_create_info.pCode = (u32*)buffer;
    if(vkCreateShaderModule(device, &shader_module_create_info, 0, shader_module) != VK_SUCCESS) {
        panic("Failed to create shader module!");
    }
}

#define UNTITLED_FPS_VKUTIL_H
#endif //UNTITLED_FPS_VKHELPERS_H
