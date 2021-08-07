//
// Created by Windows Vista on 8/6/2021.
//

#ifndef UNTITLED_FPS_STATE_H
#include <GLFW/glfw3.h>
#include "util.h"

typedef struct Graphics {
    const REQUIRED_LAYER_COUNT = 1;
    const char* REQUIRED_LAYER_NAMES[REQUIRED_LAYER_COUNT] = {
        "VK_LAYER_KHRONOS_validation",
    };

//TODO(sean): check that this is supported
    const REQUIRED_EXTENSION_COUNT = 1;
    const char* REQUIRED_EXTENSION_NAMES[REQUIRED_EXTENSION_COUNT] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    const u32 MAX_FRAMES_IN_FLIGHT = 2;
    b32 validation_enabled = 0;
    u32 supports_validation;

    u32 window_width = 800;
    u32 window_height = 600;
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;
    VkSurfaceKHR surface;
    VkQueue present_queue;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    u32 queue_family_indices[2];
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkExtent2D surface_extent;
    VkSwapchainKHR swapchain;
    u32 swapchain_image_count;
    StagedBuffer swapchain_buffer;
    VkImage* swapchain_images;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    VkImageView* swapchain_image_views;
    VkFramebuffer* swapchain_framebuffers;
    VkCommandPool command_pool;
} Graphics;

typedef struct State {


} State;

#define UNTITLED_FPS_STATE_H

#endif //UNTITLED_FPS_STATE_H
