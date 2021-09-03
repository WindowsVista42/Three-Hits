//
// Created by Windows Vista on 8/6/2021.
//

#ifndef UNTITLED_FPS_STATE_H
#include <GLFW/glfw3.h>
#include "util.h"
#include "vmmath.h"
#include "alutil.h"
#include "input.h"

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

#define medium_vertex_count 4
global Vertex medium_vertices[medium_vertex_count] = {
    {{{-1.0, 0.0, -1.0}}, {{0.0, 1.0, 0.0}}, {{0.0, 0.0}}},
    {{{-1.0, 0.0,  1.0}}, {{0.0, 1.0, 0.0}}, {{0.0, 1.0}}},
    {{{ 1.0, 0.0, -1.0}}, {{0.0, 1.0, 0.0}}, {{1.0, 0.0}}},
    {{{ 1.0, 0.0,  1.0}}, {{0.0, 1.0, 0.0}}, {{1.0, 1.0}}},
};

#define small_vertex_count 4
global Vertex small_vertices[small_vertex_count] = {
    {{{-0.5, 0.0, -0.5}}, {{0.0, 1.0, 0.0}}, {{0.0, 0.0}}},
    {{{-0.5, 0.0,  0.5}}, {{0.0, 1.0, 0.0}}, {{0.0, 1.0}}},
    {{{ 0.5, 0.0, -0.5}}, {{0.0, 1.0, 0.0}}, {{1.0, 0.0}}},
    {{{ 0.5, 0.0,  0.5}}, {{0.0, 1.0, 0.0}}, {{1.0, 1.0}}},
};

#define large_vertex_count 4
global Vertex large_vertices[large_vertex_count] = {
    {{{-2.0, 0.0, -2.0}}, {{0.0, 1.0, 0.0}}, {{0.0, 0.0}}},
    {{{-2.0, 0.0,  2.0}}, {{0.0, 1.0, 0.0}}, {{0.0, 1.0}}},
    {{{ 2.0, 0.0, -2.0}}, {{0.0, 1.0, 0.0}}, {{1.0, 0.0}}},
    {{{ 2.0, 0.0,  2.0}}, {{0.0, 1.0, 0.0}}, {{1.0, 1.0}}},
};

#define entity_index_count 6
global u32 enemy_indices[entity_index_count] = {
    0, 1, 2,
    2, 1, 3
};

typedef struct UniformBufferObject {
    mat4 view_projection;
} UniformBufferObject;

#define descriptor_count 3
#define light_count 6

#define KEYCARD_NONE 0x00000000
#define KEYCARD_RED 0x00000001
#define KEYCARD_BLUE 0x00000002
#define KEYCARD_YELLOW 0x00000004
#define KEYCARD_GREEN 0x00000008

/*  We get a maximum of 4 descriptor sets if we want to run on intel igpus.
*/

typedef struct EntityList {
    Model model;
    Texture texture;
    VkDescriptorSet descriptor_set;

    u32 capacity;
    u32 length;

    vec4* position_rotations;
    Buffer position_rotation_buffer;
    Buffer position_rotation_staging_buffer;

    vec4* colors;
    Buffer color_buffer;
    Buffer color_staging_buffer;
} EntityList;

typedef struct EnemyList {
    EntityList entities;

    i32* healths;
    f32* hit_times;
    f32* shoot_times;
    b32* sees_player;
    b32* reverse_windup;

    f32 activation_range;
    f32 shoot_range;
    f32 shoot_delay;
    f32 radius;
    f32 speed;
    f32 hit_reaction_duration;

    SoundSource* alert_sound_sources;
    SoundSource* ambience_sound_sources;
    SoundSource* gun_sound_sources;
    SoundSource* windup_sound_sources;
} EnemyList;

typedef struct EnemySoundBuffers {
    SoundBuffer alert;
    SoundBuffer ambience;
    SoundBuffer explosion;
    SoundBuffer gun;
    SoundBuffer windup;
    SoundBuffer winddown;
} EnemySoundBuffers;

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

    StagedBuffer level_buffer;
    StagedBuffer level_scratch_buffer;

    Model level_model;
    Texture level_texture;

    VkDescriptorPool global_descriptor_pool;

    VkDescriptorSetLayout global_descriptor_set_layout;
    VkDescriptorSet* global_descriptor_sets; // shared data
    Buffer* camera_uniforms;

    u32 ulight_count;
    Light* initial_lights;
    Light* lights;
    Buffer light_staging_buffer;
    Buffer light_buffer;

    EnemyList mediums;
    EnemyList rats; // delay 1.0, range 2.0
    EnemyList knights; // delay 2.0, range larger

    b32 sounds_loaded;
    EnemySoundBuffers medium_sounds;
    EnemySoundBuffers rat_sounds;
    EnemySoundBuffers knight_sounds;


    f32 mouse_sensitivity;
    vec2 mouse_pos;
    vec2 mouse_delta;
    f32 theta;
    f32 phi;

    f32 elapsed_time;
    f32 delta_time;

    // Physics

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

    //u32 door_count;
    EntityList doors;
    u32 door_physmesh_range_count;
    u32* door_physmesh_ranges;
    f32* door_timings;
    u32* door_requirements;

    //
    u32 player_keycards;
    EntityList keycards;

    //
    vec3 player_position;
    vec3 look_dir;
    f32 player_speed;
    f32 player_jump_speed;
    f32 player_radius;
    f32 player_z_speed;

    // crosshair
    //TODO(sean): move this to a uniform
    Modules crosshair_modules;
    Pipeline crosshair_pipeline;
    vec4 crosshair_color;
    Buffer crosshair_color_buffer;
    Buffer crosshair_color_staging_buffer;

    // audio
    AlDevice* al_device;
    AlContext* al_context;
    ReverbProperties reverb;
    SoundEffect reverb_effect;
    SoundSlot reverb_slot;

    SoundBuffer pistol_sound_buffer;
    SoundSource player_gun_sound_source;
    SoundSource player_movement_sound_source;
    SoundBuffer player_movement_sound_buffer;
    SoundBuffer player_jump_sound_buffer;

    SoundBuffer door_opening_sound_buffer;
    SoundSource* door_sound_sources;

    // guns
    f32 pistol_shoot_delay;
    f32 pistol_reload_speed;
    u32 loaded_pistol_ammo_count;
    u32 pistol_magazine_size;

    i32 player_health;

    b32 load_next_level;
    u32 level_index;

    vec4 end_zone;
    vec4 start_zone;

    // Input keybinds
    Bind shoot_button;

    Bind activate_key;
    Bind reload_key;
    Bind forward_key;
    Bind backward_key;
    Bind left_key;
    Bind right_key;
    Bind walk_key;
    Bind crouch_key;
    Bind jump_key;

    Bind debug_xp_key;
    Bind debug_xn_key;
    Bind debug_yp_key;
    Bind debug_yn_key;
    Bind debug_zp_key;
    Bind debug_zn_key;
    Bind debug_wp_key;
    Bind debug_wn_key;
    Bind debug_next_key;
    Bind debug_mode_key;
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

typedef struct ConfigState {
} ConfigState;

#define UNTITLED_FPS_STATE_H

#endif //UNTITLED_FPS_STATE_H
