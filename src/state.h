//
// Created by Windows Vista on 8/6/2021.
//

#ifndef UNTITLED_FPS_STATE_H
#include <GLFW/glfw3.h>
#include "util.h"
#include "vmmath.h"

typedef struct Vertex {
    vec3 pos;
    vec3 color;
    vec2 uv;
} Vertex;

#define vertex_binding_description_count 1
global VkVertexInputBindingDescription vertex_binding_descriptions[vertex_binding_description_count] = {
    {   .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    }
};
#define vertex_attribute_description_count 3
global VkVertexInputAttributeDescription vertex_attribute_descriptions[vertex_attribute_description_count] = {
    {   .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, pos),
    },
    {   .binding = 0,
        .location = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, color),
    },
    {   .binding = 0,
        .location = 2,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, uv),
    }
};

typedef struct UniformBufferObject {
    mat4 model;
    mat4 view_proj;
} UniformBufferObject;

#define descriptor_count 2
typedef struct State {
    u32 window_width;
    u32 window_height;
    GLFWwindow *window;
    GLFWmonitor *primary_monitor;

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
    VkImage *swapchain_images;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    VkImageView *swapchain_image_views;
    VkFramebuffer *swapchain_framebuffers;
    VkCommandPool command_pool;
    VkFormat depth_image_format;
    b32 depth_image_has_stencil;
    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;

    VkImage texture_image;
    VkDeviceMemory texture_image_memory;
    VkImageView texture_image_view;
    VkSampler texture_image_sampler;

    VkShaderModule vertex_module;
    VkShaderModule fragment_module;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;
    VkPipeline graphics_pipeline;
    VkCommandBuffer* command_buffers;

    StagedBuffer semaphore_buffer;
    VkSemaphore* image_available_semaphores;
    VkSemaphore* render_finished_semaphores;
    VkFence* in_flight_fences;
    VkFence* image_in_flight_fences;
    usize current_frame_index;

    DebugCallbackData debug_callback_data;
    StagedBuffer scratch;

    f32 model_rotation;
    f32 model_rotation_offset;
    vec3 model_position;

    // We only want to load this temporarily into memory
    //usize model_vertices_count;
    //f32* model_vertices;
    //usize model_indices_count;
    //u16* model_indices;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;

    VkBuffer* uniform_buffers;
    VkDeviceMemory* uniform_buffers_memory;
    VkDescriptorPool uniform_descriptor_pool;

    VkDescriptorSet* descriptor_sets;

    vec3 player_eye;
    vec3 look_dir;
    vec2 mouse_pos;
    vec2 mouse_delta;
    f32 theta;
    f32 phi;

    f32 elapsed_time;
    f32 delta_time;

    f32 mouse_sensitivity;
    f32 player_speed;
} State;

#define UNTITLED_FPS_STATE_H

#endif //UNTITLED_FPS_STATE_H
