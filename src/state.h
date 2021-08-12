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

//TODO(sean): use this instead of Vertex
typedef struct DataVertex {
    vec3 position;
    u32 texture_indices; // packed u16 + u16
    vec2 texture_uv;
    vec2 lightmap_uv;
} DataVertex;

#define data_vertex_binding_description_count 1
global VkVertexInputBindingDescription data_vertex_binding_descriptions[data_vertex_binding_description_count] = {
    {   .binding = 0,
        .stride = sizeof(DataVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    }
};
#define data_vertex_attribute_description_count 4
global VkVertexInputAttributeDescription data_vertex_attribute_descriptions[data_vertex_attribute_description_count] = {
    {   .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(DataVertex, position),
    },
    {   .binding = 0,
        .location = 1,
        .format = VK_FORMAT_R16G16_UINT,
        .offset = offsetof(DataVertex, texture_indices),
    },
    {   .binding = 0,
        .location = 2,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(DataVertex, texture_uv)
    },
    {   .binding = 0,
        .location = 3,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(DataVertex, lightmap_uv)
    },
};

typedef struct ModelPosition {
    vec3 position;
} ModelPosition;

#define model_position_binding_description_count 1
global VkVertexInputBindingDescription model_position_binding_descriptions[model_position_binding_description_count] = {
    {   .binding = 1,
        .stride = sizeof(ModelPosition),
        .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
    }
};

#define model_position_attribute_description_count 1
global VkVertexInputAttributeDescription model_position_attribute_descriptions[model_position_attribute_description_count] = {
    {   .binding = 1,
        .location = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(ModelPosition, position),
    },
};

typedef struct UniformBufferObject {
    mat4 model;
    mat4 view_proj;
} UniformBufferObject;

typedef struct Buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
} Buffer;

typedef struct ModelBuffer {
    Buffer vertices;
    u32 index_count;
    Buffer indices;
} ModelBuffer;

typedef struct Texture {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
} Texture;

#define descriptor_count 2
typedef struct GameState {
    u32 window_width;
    u32 window_height;
    GLFWwindow *window;
    GLFWmonitor *primary_monitor;
    b32 window_fullscreen;

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
    Texture depth_texture;

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

    // World model
    VkSampler generic_sampler;

    ModelBuffer level_model;
    Texture level_texture;

    VkBuffer* uniform_buffers;
    VkDeviceMemory* uniform_buffers_memory;
    VkDescriptorPool uniform_descriptor_pool;

    VkDescriptorSet* descriptor_sets;

    // Enemy model
    u32 enemy_count;
    ModelBuffer enemy_model;
    Texture enemy_texture;
    Buffer enemy_position_buffer;

    f32 mouse_sensitivity;
    vec2 mouse_pos;
    vec2 mouse_delta;
    f32 theta;
    f32 phi;

    f32 elapsed_time;
    f32 delta_time;

    // Physics
    StagedBuffer physics_buffer;
    StagedBuffer physics_scratch_buffer;

    f32 gravity;
    f32 sliding_threshold;

    u32 level_physmesh_vertex_count;
    vec3* level_physmesh;

    vec3 player_position;
    vec3 look_dir;
    f32 player_speed;
    f32 player_height;
    f32 player_radius;
    f32 player_z_speed;

    //u32 enemies_count;
    vec3* enemy_positions;
    f32 enemy_radii;
    f32* enemy_z_speeds;

    // we also need a render pass for enemies
} GameState;

#include "../lib/stb_image.h"

// We only want to load this temporarily into memory
typedef struct LoaderState {
    StagedBuffer scratch;
    StagedBuffer read_scratch;

    // Level texture data
    char* texture_path;
    int texture_width;
    int texture_height;
    int texture_channels;
    stbi_uc* texture_pixels;
    VkDeviceSize image_size;

    // Level vertex and index data
    char* level_path;
    u32 level_vertex_count;
    Vertex* level_vertices;
    u32 level_index_count;
    u32* level_indices;
} LoaderState;

typedef struct ProjType_s
{
    u8 r, g, b;
    f32 base_radius, max_speed;
} ProjType;

typedef struct Proj_s {
    int type;
    f32 x, y;
    f32 vx, vy;
    f32 mass, radius;
} Proj;

#define UNTITLED_FPS_STATE_H

#endif //UNTITLED_FPS_STATE_H
