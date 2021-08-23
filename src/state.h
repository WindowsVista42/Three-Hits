//
// Created by Windows Vista on 8/6/2021.
//

#ifndef UNTITLED_FPS_STATE_H
#include <GLFW/glfw3.h>
#include "util.h"
#include "vmmath.h"
#include "alutil.h"

typedef struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
} Vertex;

typedef struct DataVertex {
    vec3 position;
    vec3 normal;
    vec3 texture_uvi;
    vec3 lightmap_uvi;
} DataVertex;

typedef struct Light {
    vec4 position_falloff;
    vec4 color_alpha;
} Light;

#define level_vertex_input_binding_description_count 1
global VkVertexInputBindingDescription vertex_binding_descriptions[level_vertex_input_binding_description_count] = {
    {   .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    }
};
#define level_vertex_attribute_description_count 3
global VkVertexInputAttributeDescription vertex_attribute_descriptions[level_vertex_attribute_description_count] = {
    {   .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, position),
    },
    {   .binding = 0,
        .location = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, normal),
    },
    {   .binding = 0,
        .location = 2,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, uv),
    }
};

#define entity_vertex_binding_description_count 3
global VkVertexInputBindingDescription enemy_vertex_binding_descriptions[entity_vertex_binding_description_count] = {
    {   .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    },
    {   .binding = 1,
        .stride = sizeof(vec4),
        .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
    },
    {   .binding = 2,
        .stride = sizeof(vec4),
        .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
    }
};

#define entity_vertex_attribute_description_count 5
global VkVertexInputAttributeDescription enemy_vertex_attribute_descriptions[entity_vertex_attribute_description_count] = {
    {   .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, position),
    },
    {   .binding = 0,
        .location = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, normal),
    },
    {   .binding = 0,
        .location = 2,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, uv),
    },
    {   .binding = 1,
        .location = 3,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = 0,
    },
    {   .binding = 2,
        .location = 4,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = 0,
    },
};

#define crosshair_vertex_binding_description_count 1
global VkVertexInputBindingDescription crosshair_vertex_binding_descriptions[entity_vertex_binding_description_count] = {
    {   .binding = 0,
        .stride = sizeof(vec4),
        .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
    },
};

#define crosshair_vertex_attribute_description_count 1
global VkVertexInputAttributeDescription crosshair_vertex_attribute_descriptions[entity_vertex_attribute_description_count] = {
    {   .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = 0,
    },
};

#define enemy_vertex_count 4
global Vertex enemy_vertices[enemy_vertex_count] = {
    {{{-1.0, 0.0, -1.0}}, {{0.0, 1.0, 0.0}}, {{0.0, 0.0}}},
    {{{-1.0, 0.0,  1.0}}, {{0.0, 1.0, 0.0}}, {{0.0, 1.0}}},
    {{{ 1.0, 0.0, -1.0}}, {{0.0, 1.0, 0.0}}, {{1.0, 0.0}}},
    {{{ 1.0, 0.0,  1.0}}, {{0.0, 1.0, 0.0}}, {{1.0, 1.0}}},
};

#define enemy_index_count 6
global u32 enemy_indices[enemy_index_count] = {
    0, 1, 2,
    2, 1, 3
};

typedef struct UniformBufferObject {
    mat4 view_projection;
} UniformBufferObject;

#define descriptor_count 3
#define light_count 6

/*  We get a maximum of 4 descriptor sets if we want to run on intel igpus.
*/

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
    Texture depth_texture;
    u32 current_image;

    Modules level_modules;
    Pipeline level_pipeline;

    Modules entity_modules;
    Pipeline entity_pipeline;

    VkCommandBuffer* command_buffers;

    StagedBuffer semaphore_buffer;
    VkSemaphore* image_available_semaphores;
    VkSemaphore* render_finished_semaphores;
    VkFence* in_flight_fences;
    VkFence* image_in_flight_fences;
    usize current_frame_index;

    DebugCallbackData debug_callback_data;
    StagedBuffer scratch;

    // World model
    VkSampler generic_sampler;

    Model level_model;
    Texture level_texture;

    VkDescriptorPool global_descriptor_pool;

//    VkBuffer* uniform_buffers;
//    VkDeviceMemory* uniform_buffers_memory;
//    VkDescriptorSet* descriptor_sets;

    //TODO(sean): figure out if this is *very* specific to pipelines
    VkDescriptorSetLayout ubo_sampler_descriptor_set_layout;

    VkDescriptorSet* global_descriptor_sets; // shared data
    Buffer* camera_uniforms;
    Buffer light_buffer;

//
    u32 max_enemy_count;
    u32 enemy_alive_count;

    StagedBuffer enemy_buffer;

    Model enemy_model;
    Texture enemy_texture;
    VkDescriptorSet enemy_descriptor_set;
    Buffer enemy_position_rotation_buffer;
    Buffer enemy_position_rotation_staging_buffer;

    i32* enemy_healths;
    f32* enemy_hit_times;
    f32* enemy_shoot_times;
    f32 enemy_shoot_delay;

    vec4* enemy_colors;
    Buffer enemy_color_buffer;
    Buffer enemy_color_staging_buffer;

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
    f32 slide_gravity_factor;

    u32 physmesh_vertex_count;
    vec3* physmesh_vertices;
    u32 closest_ray_index;

    // doors
    f32 door_activation_distance;
    f32 max_door_open_time;
    f32 door_move_speed;

    u32 door_count;
    u32* door_requirements; // 0 = none, 1+ = some programmable event
    vec4* door_position_rotations;
    vec4* door_colors;
    f32* door_timings;

    u32 door_physmesh_range_count;
    u32* door_physmesh_ranges;

    Model door_model;
    Buffer door_color_buffer;
    Buffer door_color_staging_buffer;
    Buffer door_position_rotation_buffer;
    Buffer door_position_rotation_staging_buffer;

    //
    u32 max_keycard_count;
    u32 keycard_count;
    Buffer keycard_position_rotation_buffer;
    Buffer keycard_position_rotation_staging_buffer;
    vec4* keycard_position_rotations;

    //
    vec3 player_position;
    vec3 look_dir;
    f32 player_speed;
    f32 player_jump_speed;
    f32 player_radius;
    f32 player_z_speed;

    vec4* enemy_position_rotations;

    // crosshair
    //TODO(sean): move this to a uniform
    Modules crosshair_modules;
    Pipeline crosshair_pipeline;
    vec4 crosshair_color;
    Buffer crosshair_color_buffer;
    Buffer crosshair_color_staging_buffer;

    // audio
    StagedBuffer audio_buffer;

    AlDevice* al_device;
    AlContext* al_context;
    ReverbProperties reverb;
    SoundEffect reverb_effect;
    SoundSlot reverb_slot;

    SoundBuffer pistol_sound_buffer;
    SoundBuffer enemy_alert_sound_buffer;
    SoundBuffer enemy_ambience_sound_buffer;
    SoundBuffer enemy_explosion_sound_buffer;
    SoundBuffer enemy_gun_sound_buffer;
    SoundBuffer door_opening_sound_buffer;
    SoundBuffer player_movement_sound_buffer;
    SoundBuffer player_jump_sound_buffer;
    SoundBuffer enemy_windup_sound_buffer;
    SoundBuffer enemy_reverse_windup_sound_buffer;

    SoundSource player_gun_sound_source;
    SoundSource player_movement_sound_source;

    SoundSource* enemy_alert_sound_sources;
    SoundSource* enemy_ambience_sound_sources;
    SoundSource* enemy_gun_sound_sources;
    SoundSource* door_sound_sources;
    SoundSource* enemy_windup_sound_sources;
    b32* windup_needs_reverse;
    b32* enemy_sees_player;

    // guns
    f32 pistol_shoot_delay;
    f32 pistol_reload_speed;
    u32 loaded_pistol_ammo_count;
    u32 pistol_magazine_size;

    f32 enemy_simulation_radius;

    i32 player_health;
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

    u32 door_vertex_count;
    Vertex* door_vertices;
    u32 door_index_count;
    u32* door_indices;
} LoaderState;

#define UNTITLED_FPS_STATE_H

#endif //UNTITLED_FPS_STATE_H
