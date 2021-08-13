#define GLFW_INCLUDE_VULKAN

// std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// src
#include "util.h"
#include "vkutil.h"
#include "vmmath.h"
#include "state.h"
#include "physics.h"

// lib
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

#define REQUIRED_LAYER_COUNT 1
global const char* REQUIRED_LAYER_NAMES[REQUIRED_LAYER_COUNT] = {
        "VK_LAYER_KHRONOS_validation",
};
//NOTE(sean): If we dont support this then we just shouldnt run
#define REQUIRED_EXTENSION_COUNT 1
global const char* REQUIRED_EXTENSION_NAMES[REQUIRED_EXTENSION_COUNT] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

global const u32 MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
global b32 validation_enabled = 1;
#else
global b32 validation_enabled = 0;
#endif

global u32 supports_validation;

global GameState state;
global LoaderState* loader;

void find_depth_image_format() {
    VkFormat formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    state.depth_image_format = find_supported_format(state.physical_device, 3, formats,
                                                     VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    state.depth_image_has_stencil = state.depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT || state.depth_image_format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void create_depth_image() {
    create_image(state.device, state.physical_device, state.swapchain_extent.width, state.swapchain_extent.height, state.depth_image_format,
                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &state.depth_image, &state.depth_image_memory);
    create_image_view(state.device, state.depth_image, state.depth_image_format, VK_IMAGE_ASPECT_DEPTH_BIT, &state.depth_image_view);
}

//TODO(sean): move some of this into a custom function
void create_texture_image() {

    create_device_local_image(state.device, state.physical_device, state.queue, state.command_pool,
                              loader->texture_width, loader->texture_height, loader->image_size, loader->texture_pixels,
                              VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                              &state.level_texture.image, &state.level_texture.memory);

    create_image_view(state.device, state.level_texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, &state.level_texture.view);
}

void create_generic_sampler() {
    create_image_sampler(state.device, state.physical_device, &state.generic_sampler);
}

//TODO(sean): move statics out
void update_uniforms(u32 current_image) {
    UniformBufferObject ubo = {};
    mat4 projection, view;

    {
        f32 wall_distance = 1024.0;
        vec3 E = vec3_add_vec3(state.player_position, vec3_mul_f32(vec3_mul_f32(state.look_dir, -1.0), 64.0));

        for (usize index = 0; index < state.level_physmesh_vertex_count; index += 3) {
            vec3 A = state.level_physmesh[index + 0];
            vec3 B = state.level_physmesh[index + 1];
            vec3 C = state.level_physmesh[index + 2];
            vec3 P = state.player_position;
            vec3 N;
            f32 d;

            // distance for closest intersection from S to E
            if (ray_intersects_triangle(A, B, C, P, E, &N, &d) == true) {
                if (d < wall_distance) {
                    wall_distance = d;
                }
            }
        }

        u32 hit_index = UINT32_MAX;
        f32 best_enemy_distance = 1024.0;
        if (glfwGetMouseButton(state.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            for (usize index = 0; index < state.enemy_count; index += 1) {
                vec3 Pe = *(vec3 *) &state.enemy_position_rotations[index].x;
                vec3 P = state.player_position;
                vec3 enemyN;
                f32 enemy_distance;
                f32 r = 1.8;
                f32 rr = r * r;

                // we know that we intersect the enemy
                if (line_intersects_sphere(P, E, Pe, rr, &enemyN, &enemy_distance) == true) {
                    if (enemy_distance < wall_distance && enemy_distance < best_enemy_distance) {
                        best_enemy_distance = enemy_distance;
                        hit_index = index;
                    }
                }
            }
        }

        if (hit_index != UINT32_MAX) {
            state.enemy_position_rotations[hit_index].x += 5.0 * state.delta_time;
        }

        for(usize index = 0; index < state.enemy_count; index += 1) {
            state.enemy_position_rotations[index].w = atan2f(state.player_position.x - state.enemy_position_rotations[index].x,
                                                         state.player_position.y - state.enemy_position_rotations[index].y);
        }
        u64 copy_size = state.enemy_count * sizeof(vec4);

        write_buffer(state.device, state.enemy_position_rotation_staging_buffer.memory, 0, copy_size, 0, state.enemy_position_rotations);

        copy_buffer_to_buffer(
            state.device,
            state.queue,
            state.command_pool,
            state.enemy_position_rotation_staging_buffer.buffer,
            state.enemy_position_rotation_buffer.buffer,
            copy_size
        );
    }

    b32 space_pressed = false;
    static b32 space_held = false;
    static b32 airborne = true;
    vec3 xydir = {{state.look_dir.x, state.look_dir.y, 0.0}};
    xydir = vec3_norm(xydir);
    vec3 normal = {{-xydir.y, xydir.x, 0.0}};
    vec3 delta = VEC3_ZERO;
    f32 player_speed = state.player_speed;
    f32 player_radius = state.player_radius;
    {
        if(glfwGetKey(state.window, GLFW_KEY_W) == GLFW_PRESS) {
            delta = vec3_sub_vec3(delta, xydir);
        }
        if(glfwGetKey(state.window, GLFW_KEY_A) == GLFW_PRESS) {
            delta = vec3_sub_vec3(delta, normal);
        }
        if(glfwGetKey(state.window, GLFW_KEY_S) == GLFW_PRESS) {
            delta = vec3_add_vec3(delta, xydir);
        }
        if(glfwGetKey(state.window, GLFW_KEY_D) == GLFW_PRESS) {
            delta = vec3_add_vec3(delta, normal);
        }
        if(glfwGetKey(state.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            player_speed /= 2.0;
        } else if(glfwGetKey(state.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            player_speed /= 2.0;
            player_radius /= 2.0;
        }


        if(glfwGetKey(state.window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            space_pressed = true;
        }

        state.theta = f32_wrap(state.theta, 2.0 * M_PI);

        state.look_dir = vec3_from_theta_phi(state.theta, state.phi);
    }

    state.delta_time /= 12.0;
    for(usize i = 0; i < 12; i += 1) {
        if(space_pressed && !space_held && airborne) {
            state.player_z_speed = -state.player_jump_speed;
            state.player_position.z -= state.player_z_speed * state.delta_time;
            space_held = true;
            airborne = false;
        } else {
            state.player_position.z -= state.player_z_speed * state.delta_time;
            space_held = false;
        }

        if(vec3_par_ne_vec3(delta, VEC3_ZERO)) {
            delta = vec3_norm(delta);
            delta = vec3_mul_f32(delta, state.delta_time);
            delta = vec3_mul_f32(delta, player_speed);
            state.player_position = vec3_add_vec3(state.player_position, delta);
        }

        static u32 count = 0;
        count += 1;
        state.player_z_speed += state.gravity * state.delta_time;
        {
            static b32 hit_head = false;

            u32 vertex_count = 0;
            vec3* vertices = sbmalloc(&state.physics_scratch_buffer, 1200 * sizeof(vec3));

            f32 r = player_radius;
            f32 rr = r * r;
            vec3 N;
            f32 d;

            for(usize index = 0; index < state.level_physmesh_vertex_count; index += 3) {
                vec3 A = state.level_physmesh[index + 0];
                vec3 B = state.level_physmesh[index + 1];
                vec3 C = state.level_physmesh[index + 2];
                vec3 P = state.player_position;

                // player intersection culling
                vec3 ABC = vec3_div_f32(vec3_add_vec3(vec3_add_vec3(A, B), C), 3.0);
                f32 ABCrr = vec3_distsq_vec3(ABC, A);
                f32 ABCBrr = vec3_distsq_vec3(ABC, B);
                f32 ABCCrr = vec3_distsq_vec3(ABC, C);
                if(ABCBrr > ABCrr) { ABCrr = ABCBrr; }
                if(ABCCrr > ABCrr) { ABCrr = ABCCrr; }
                if(vec3_distsq_vec3(ABC, P) < ABCrr + rr) {
                    vertices[vertex_count + 0] = A;
                    vertices[vertex_count + 1] = B;
                    vertices[vertex_count + 2] = C;
                    vertex_count += 3;
                }
            }

            // Check if we are colliding with any of the triangles
            for (usize index = 0; index < vertex_count; index += 3) {
                vec3 A = vertices[index + 0];
                vec3 B = vertices[index + 1];
                vec3 C = vertices[index + 2];
                vec3 P = state.player_position;

                if(sphere_collides_with_triangle(A, B, C, P, rr, &N, &d)) {
                    d += (1.0 / 512.0);
                    f32 sliding_factor = vec3_dot(N, VEC3_UNIT_Z);

                    if(sliding_factor > state.sliding_threshold) {
                        N = VEC3_UNIT_Z;
                        state.player_z_speed = state.gravity * state.slide_gravity_factor * state.delta_time;
                        airborne = true;
                        hit_head = false;
                    } else if (sliding_factor < -state.sliding_threshold && !airborne && !hit_head) {
                        state.player_z_speed = 0.0f;
                        hit_head = true;
                    }
                    if(vec3_eq_vec3(N, VEC3_ZERO)) { N = VEC3_UNIT_Z; }
                    state.player_position = vec3_add_vec3(state.player_position, vec3_mul_f32(N, d));
                }
            }

            sbclear(&state.physics_scratch_buffer);
        }
    }
    state.delta_time *= 12.0;

    view = mat4_look_dir(state.player_position, state.look_dir, VEC3_UNIT_Z);
    projection = mat4_perspective(f32_radians(100.0f), (float)state.swapchain_extent.width / (float)state.swapchain_extent.height, 0.01f, 1000.0f);
    ubo.view_proj = mat4_mul_mat4(view, projection);
    write_buffer(state.device, state.uniform_buffers_memory[current_image], 0, sizeof(ubo), 0, &ubo);
}

void recreate_swapchain();

void update_and_render() {
    vkWaitForFences(state.device, 1, &state.in_flight_fences[state.current_frame_index], VK_TRUE, UINT64_MAX);

    u32 image_index;
    VkResult result;

    result = vkAcquireNextImageKHR(state.device, state.swapchain, UINT64_MAX, state.image_available_semaphores[state.current_frame_index], 0, &image_index);

    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        //TODO(sean): diagnostic
        panic("Failed to acquire next swapchain image!");
    }

    update_uniforms(image_index);

    if(state.image_in_flight_fences[state.current_frame_index] != 0) {
        vkWaitForFences(state.device, 1, &state.image_in_flight_fences[state.current_frame_index], VK_TRUE, UINT64_MAX);
    }
    state.image_in_flight_fences[state.current_frame_index] = state.in_flight_fences[state.current_frame_index];

    VkSemaphore wait_semaphores[] = {state.image_available_semaphores[state.current_frame_index]};
    VkSemaphore signal_semaphores[] = {state.render_finished_semaphores[state.current_frame_index]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &state.command_buffers[image_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkResetFences(state.device, 1, &state.in_flight_fences[state.current_frame_index]);

    if(vkQueueSubmit(state.queue, 1, &submit_info, state.in_flight_fences[state.current_frame_index]) != VK_SUCCESS) {
        //TODO(sean): diagnostic
        panic("Failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapchains[] = {state.swapchain};
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = 0;

    result = vkQueuePresentKHR(state.present_queue, &present_info);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreate_swapchain();
    } else if (result != VK_SUCCESS) {
        //TODO(sean): diagnostic
        panic("Failed to present swapchain image!");
    }

    state.current_frame_index += 1;
    state.current_frame_index %= MAX_FRAMES_IN_FLIGHT;
}

global void cursor_position_callback(GLFWwindow* pwindow, double xpos, double ypos) {
    f32 lastx, lasty;
    lastx = state.mouse_pos.x;
    lasty = state.mouse_pos.y;

    state.mouse_pos.x = (f32)xpos / 1024.0;
    state.mouse_pos.y = (f32)ypos / 1024.0;

    state.mouse_delta.x = lastx - state.mouse_pos.x;
    state.mouse_delta.y = lasty - state.mouse_pos.y;

    state.theta += state.mouse_delta.y * state.mouse_sensitivity;
    state.phi += state.mouse_delta.x * state.mouse_sensitivity;

    state.theta = f32_clamp(state.theta, 0.01, M_PI - 0.01);
}

void init_window() {
    // init glfw window
    if(!glfwInit()) { panic("Failed to initialize GLFW (somehow?)!"); };
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    state.window = glfwCreateWindow(state.window_width, state.window_height, "Spinning Cube", state.primary_monitor, 0);

    glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if(glfwRawMouseMotionSupported()) {
        glfwSetInputMode(state.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    glfwSetCursorPosCallback(state.window, cursor_position_callback);
}

// check layers support
// TODO(sean): move this to compile flag
void check_layers_support() {
    if(!validation_enabled) return;

    u32 layer_count;
    VkLayerProperties* available_layers;
    // TODO(sean): move this to compile flag

    vkEnumerateInstanceLayerProperties(&layer_count, 0);
    available_layers = sbmalloc(&state.scratch, layer_count * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for(usize index = 0; index < layer_count; index += 1) {
        VkLayerProperties* current_layer = available_layers + index;

        for(usize layer_index = 0; layer_index < REQUIRED_LAYER_COUNT; layer_index += 1) {
            const char* layer_name = REQUIRED_LAYER_NAMES[layer_index];
            if(strcmp(layer_name, current_layer->layerName) == 0) {
                supports_validation = 1;
                break;
            }
        }
    }

    sbclear(&state.scratch);
}

void init_instance() {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "Synapse";
    app_info.engineVersion = VK_MAKE_VERSION(3, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};

    u32 glfw_ext_count = 0;
    const char** glfw_ext;
    glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    // TODO(sean): move this to compile flag
    if(supports_validation && validation_enabled) {
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        debug_create_info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debug_callback;
        debug_create_info.pUserData = &state.debug_callback_data;

        create_info.enabledLayerCount = REQUIRED_LAYER_COUNT;
        create_info.ppEnabledLayerNames = REQUIRED_LAYER_NAMES;
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;

        // GLFW frees the old glfw_ext so we don't need to do that.
        // We have to create our own, because realloc will *likely* move the location of the pointer.
        // GLFW then promptly throws a hissy-fit when we try to free this.
        usize new_count = glfw_ext_count + REQUIRED_LAYER_COUNT;
        const char** new_glfw_ext = sbmalloc(&state.scratch, new_count * sizeof(char**));
        memcpy(new_glfw_ext, glfw_ext, new_count * sizeof(char*));
        glfw_ext = new_glfw_ext;
        glfw_ext[glfw_ext_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        glfw_ext_count = new_count;
    } else {
        create_info.enabledLayerCount = 0;
        create_info.pNext = 0;
    }

    create_info.enabledExtensionCount = glfw_ext_count;
    create_info.ppEnabledExtensionNames = glfw_ext;

    if (vkCreateInstance(&create_info, 0, &state.instance) != VK_SUCCESS) {
        //TODO(sean): diagnostic
        panic("Failed to create instance!");
    }

    // setup debug messenger
    if(supports_validation && validation_enabled) {
        PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(state.instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != 0) {
            func(state.instance, &debug_create_info, 0, &state.debug_messenger);
        } else {
            //TODO(sean): diagnostic
            panic("Failed to create debug messenger!");
        }
    }

    sbclear(&state.scratch);
}

void init_surface() {
    if(glfwCreateWindowSurface(state.instance, state.window, 0, &state.surface) != 0) {
        //TODO(sean): diagnostic
        panic("Failed to create window surface!");
    }
}

void init_physical_device() {
    u32 physical_device_count = 0;
    vkEnumeratePhysicalDevices(state.instance, &physical_device_count, 0);

    if (physical_device_count == 0) {
        //TODO(sean): diagnostic
        panic("Failed to find GPUs with Vulkan support!");
    }

    VkPhysicalDevice* physical_devices = sbmalloc(&state.scratch, physical_device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(state.instance, &physical_device_count, physical_devices);

    //TODO(sean): make this algorithm smarter
    for(usize index = 0; index < physical_device_count; index += 1) {
        VkPhysicalDeviceProperties device_properties = {};
        VkPhysicalDeviceFeatures device_features = {};

        vkGetPhysicalDeviceProperties(physical_devices[index], &device_properties);
        vkGetPhysicalDeviceFeatures(physical_devices[index], &device_features);

        if(device_features.geometryShader != VK_FALSE && device_features.samplerAnisotropy != VK_FALSE) {
            state.physical_device = physical_devices[index];
            break;
        }
    }

    if(state.physical_device == 0) {
        //TODO(sean): diagnostic
        panic("Failed to find suitable GPU!");
    }

    u32 graphics_queue_index = 0;
    u32 present_queue_index = 0;
    u32 queue_family_count = 0;
    VkQueueFamilyProperties* queue_families;

    vkGetPhysicalDeviceQueueFamilyProperties(state.physical_device, &queue_family_count, 0);

    queue_families = sbmalloc(&state.scratch, physical_device_count * sizeof(VkPhysicalDevice));
    vkGetPhysicalDeviceQueueFamilyProperties(state.physical_device, &queue_family_count, queue_families);

    b32 found_good_graphics_queue = 0;
    b32 found_good_present_queue = 0;
    for(usize index = 0; index < queue_family_count; index += 1) {
        if(queue_families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_index = index;
            found_good_graphics_queue = 1;
        }

        VkBool32 present_supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(state.physical_device, index, state.surface, &present_supported);

        if(present_supported) {
            present_queue_index = index;
            found_good_present_queue = 1;
        }

        if(found_good_graphics_queue && found_good_present_queue) {
            break;
        }
    }

    //TODO(sean): see if we can improve this set implementation
    state.queue_family_indices[0] = graphics_queue_index;
    state.queue_family_indices[1] = present_queue_index;
    u32 valid_queue_family_count;
    if(graphics_queue_index == present_queue_index) {
        valid_queue_family_count = 1;
    } else {
        valid_queue_family_count = 2;
    }
    u32* unique_queue_family_indices = sbmalloc(&state.scratch, valid_queue_family_count * sizeof(u32));

    VkDeviceQueueCreateInfo* queue_create_infos = sbmalloc(&state.scratch, valid_queue_family_count * sizeof(VkDeviceQueueCreateInfo));
    usize unique_queue_family_count = 1;

    //TODO(sean): try harder to get the same index
    unique_queue_family_indices[0] = graphics_queue_index;
    for(usize index = 1; index < valid_queue_family_count; index += 1) {
        u32 unique = 1;
        for(usize inter_index = 0; inter_index < unique_queue_family_count; inter_index += 1) {
            if(inter_index == index && state.queue_family_indices[inter_index] == state.queue_family_indices[index]) {
                unique = 0;
            }
        }
        if(unique == 1) {
            unique_queue_family_indices[unique_queue_family_count] = state.queue_family_indices[index];
            unique_queue_family_count += 1;
        }
    }

    //TODO(sean): get queue family indices implementation to work with different queue indices
    f32 queue_priority = 1.0;
    for(usize index = 0; index < unique_queue_family_count; index += 1) {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = unique_queue_family_indices[index];
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        queue_create_infos[index] = queue_create_info;
    }

    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE; //TODO(sean): do proper device validation

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = unique_queue_family_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.pEnabledFeatures = &device_features;

    if(supports_validation && validation_enabled) {
        device_create_info.enabledLayerCount = REQUIRED_LAYER_COUNT;
        device_create_info.ppEnabledLayerNames = REQUIRED_LAYER_NAMES;
    } else {
        device_create_info.enabledLayerCount = 0;
        device_create_info.enabledExtensionCount = 0;
    }

    //TODO(sean): actually check that this is supported
    device_create_info.enabledExtensionCount = REQUIRED_EXTENSION_COUNT;
    device_create_info.ppEnabledExtensionNames = REQUIRED_EXTENSION_NAMES; // implicitly supported by debug layers

    if(vkCreateDevice(state.physical_device, &device_create_info, 0, &state.device) != VK_SUCCESS) {
        //TODO(sean): diagnostic
        panic("Failed to create logical device!");
    }

    vkGetDeviceQueue(state.device, graphics_queue_index, 0, &state.queue);
    vkGetDeviceQueue(state.device, present_queue_index, 0, &state.present_queue);

    sbclear(&state.scratch);
}

void pre_init_swapchain() {
    u32 surface_format_count;
    VkSurfaceFormatKHR* surface_formats;

    u32 present_mode_count;
    VkPresentModeKHR* present_modes;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state.physical_device, state.surface, &state.surface_capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(state.physical_device, state.surface, &surface_format_count, 0);

    if(surface_format_count != 0) {
        surface_formats = sbmalloc(&state.scratch, surface_format_count * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(state.physical_device, state.surface, &surface_format_count, surface_formats);
    } else {
        panic("Could not find a suitable surface format!");
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(state.physical_device, state.surface, &present_mode_count, 0);

    if(present_mode_count != 0) {
        present_modes = sbmalloc(&state.scratch, surface_format_count * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(state.physical_device, state.surface, &present_mode_count, present_modes);
    } else {
        panic("Could not find a suitable present mode!");
    }

    b32 adequate = false;
    if(surface_format_count != 0 && present_mode_count != 0) {
        adequate = true;
    }

    if(adequate == false) {
        panic("Swapchain inadequate!");
    }

    // select from available surface formats and present modes
    adequate = false;
    for(usize index = 0; index < surface_format_count; index += 1) {
        //TODO(sean): figure out how i want to format this
        if(surface_formats[index].format == VK_FORMAT_B8G8R8A8_SRGB
        && surface_formats[index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            state.surface_format = surface_formats[index];
            adequate = true;
            break;
        }
    }
    if(adequate == false) {
        panic("Error finding appropriate surface format!");
    }

    adequate = false;
    for(usize index = 0; index < present_mode_count; index += 1) {
        if(present_modes[index] == VK_PRESENT_MODE_FIFO_KHR) {
            state.present_mode = present_modes[index];
            adequate = true;
            break;
        }
    }
    if(adequate == false) {
        state.present_mode = present_modes[0];
    }

    //TODO(sean): figure out how i want to format this
    if(state.surface_capabilities.currentExtent.width != UINT32_MAX && state.surface_capabilities.currentExtent.height != UINT32_MAX) {
        state.surface_extent = state.surface_capabilities.currentExtent;
    } else {
        //TODO(sean): handle this error, we need to reconstruct the swap chain size based on what glfw gives us
        panic("Failed to validate current swap chain extent!");
        /*
        i32 width, height;
        glfwGetFramebufferSize(window, &width, &height);
        swap_chain_extent.width = (u32)width;
        swap_chain_extent.height = (u32)height;
        */
    }

    state.swapchain_image_count = state.surface_capabilities.minImageCount + 1;
    if(state.surface_capabilities.maxImageCount > 0 && state.swapchain_image_count > state.surface_capabilities.maxImageCount) { // cap to max images
        state.swapchain_image_count = state.surface_capabilities.maxImageCount;
    }

    state.swapchain_format = state.surface_format.format;
    state.swapchain_extent = state.surface_extent;

    sbclear(&state.scratch);
}

void init_swapchain() {
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = state.surface;
    swapchain_create_info.minImageCount = state.swapchain_image_count;
    swapchain_create_info.imageFormat = state.surface_format.format;
    swapchain_create_info.imageColorSpace = state.surface_format.colorSpace;
    swapchain_create_info.imageExtent = state.surface_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    //swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // use this for post processing textures

    if(state.queue_family_indices[0] != state.queue_family_indices[1]) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = state.queue_family_indices;
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // this offers better performance
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = 0;
    }

    swapchain_create_info.preTransform = state.surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = state.present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = 0;


    if (vkCreateSwapchainKHR(state.device, &swapchain_create_info, 0, &state.swapchain) != VK_SUCCESS) {
        panic("Failed to create swapchain!");
    }

    vkGetSwapchainImagesKHR(state.device, state.swapchain, &state.swapchain_image_count, 0);
    state.swapchain_images = sbmalloc(&state.swapchain_buffer, state.swapchain_image_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(state.device, state.swapchain, &state.swapchain_image_count, state.swapchain_images);

    state.swapchain_image_views = sbmalloc(&state.swapchain_buffer, state.swapchain_image_count * sizeof(VkImageView));

    for(usize index = 0; index < state.swapchain_image_count; index += 1) {
        create_image_view(state.device, state.swapchain_images[index], state.swapchain_format, VK_IMAGE_ASPECT_COLOR_BIT, &state.swapchain_image_views[index]);
    }
}

void create_shader_modules() {
    usize file_count = 4;
    FILE** files = sbmalloc(&state.scratch, file_count * sizeof(FILE*));
    char* file_names[] = {"../level.vert.spv", "../level.frag.spv", "../enemy.vert.spv", "../enemy.frag.spv"};
    char** buffers = sbmalloc(&state.scratch, file_count * sizeof(char*));
    usize* buffer_sizes = sbmalloc(&state.scratch, file_count * sizeof(usize));

    for (usize index = 0; index < file_count; index += 1) {
        files[index] = fopen(file_names[index], "rb");
        panic_if_zero(files[index], "Failed to open shader code files!");
    }

    for (usize index = 0; index < file_count; index += 1) {
        FILE *fp = files[index];

        fseek(fp, 0L, SEEK_END);
        usize fsize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        buffers[index] = sbmalloc(&state.scratch, fsize);
        fread(buffers[index], fsize, 1, fp);
        buffer_sizes[index] = fsize;
    }

    for (usize index = 0; index < file_count; index += 1) {
        assert(fclose(files[index]) == 0);
    }

    create_shader_module(state.device, buffers[0], buffer_sizes[0], &state.level_modules.vertex);
    create_shader_module(state.device, buffers[1], buffer_sizes[1], &state.level_modules.fragment);

    create_shader_module(state.device, buffers[2], buffer_sizes[2], &state.enemy_modules.vertex);
    create_shader_module(state.device, buffers[3], buffer_sizes[3], &state.enemy_modules.fragment);

    sbclear(&state.scratch);
}


void create_level_graphics_pipeline() {
    PipelineOptions pipeline_options;
    pipeline_options.cull_mode = VK_CULL_MODE_BACK_BIT;
    pipeline_options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    pipeline_options.color_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    pipeline_options.color_store_op = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline_options.color_initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    pipeline_options.color_final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    pipeline_options.depth_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    pipeline_options.depth_store_op = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline_options.depth_initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    pipeline_options.depth_final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    create_graphics_pipeline(
        state.device,
        state.level_modules.vertex,
        state.level_modules.fragment,
        vertex_attribute_description_count,
        vertex_attribute_descriptions,
        level_vertex_input_binding_description_count,
        vertex_binding_descriptions,
        state.swapchain_extent.width,
        state.swapchain_extent.height,
        &state.ubo_sampler_descriptor_set_layout,
        &pipeline_options,
        &state.level_pipeline.layout,
        state.swapchain_format,
        state.depth_image_format,
        &state.level_pipeline.pass,
        &state.level_pipeline.pipeline
    );
}

void create_enemy_graphics_pipeline() {
    PipelineOptions pipeline_options;
    pipeline_options.cull_mode = VK_CULL_MODE_NONE;
    pipeline_options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    pipeline_options.color_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    pipeline_options.color_store_op = VK_ATTACHMENT_STORE_OP_STORE;
    pipeline_options.color_initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    pipeline_options.color_final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    pipeline_options.depth_load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    pipeline_options.depth_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    pipeline_options.depth_initial_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    pipeline_options.depth_final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    create_graphics_pipeline(
        state.device,
        state.enemy_modules.vertex,
        state.enemy_modules.fragment,
        enemy_vertex_attribute_description_count,
        enemy_vertex_attribute_descriptions,
        enemy_vertex_binding_description_count,
        enemy_vertex_binding_descriptions,
        state.swapchain_extent.width,
        state.swapchain_extent.height,
        &state.ubo_sampler_descriptor_set_layout,
        &pipeline_options,
        &state.enemy_pipeline.layout,
        state.swapchain_format,
        state.depth_image_format,
        &state.enemy_pipeline.pass,
        &state.enemy_pipeline.pipeline
    );
}

void create_swapchain_framebuffers() {
    state.swapchain_framebuffers = sbmalloc(&state.swapchain_buffer, state.swapchain_image_count * sizeof(VkFramebuffer));

    for (usize index = 0; index < state.swapchain_image_count; index += 1) {
        VkImageView attachments[] = {state.swapchain_image_views[index], state.depth_image_view};

        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = state.level_pipeline.pass;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = attachments;
        framebuffer_create_info.width = state.swapchain_extent.width;
        framebuffer_create_info.height = state.swapchain_extent.height;
        framebuffer_create_info.layers = 1;

        if (vkCreateFramebuffer(state.device, &framebuffer_create_info, 0, &state.swapchain_framebuffers[index]) != VK_SUCCESS) {
            //TODO(sean): diagnostic
            panic("Failed to create framebuffers!");
        }
    }
}

void init_command_pool() {
    if(state.command_pool == 0) {
        VkCommandPoolCreateInfo command_pool_create_info = {};
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.queueFamilyIndex = state.queue_family_indices[0]; // graphics queue family
        command_pool_create_info.flags = 0;

        if (vkCreateCommandPool(state.device, &command_pool_create_info, 0, &state.command_pool) != VK_SUCCESS) {
            //TODO(sean): diagnostic
            panic("Failed to create command pool!");
        }
    }

}

void create_command_buffers() {
    state.command_buffers = sbmalloc(&state.swapchain_buffer, state.swapchain_image_count * sizeof(VkCommandBuffer));

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = state.command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = (u32)state.swapchain_image_count;

    if(vkAllocateCommandBuffers(state.device, &command_buffer_allocate_info, state.command_buffers) != VK_SUCCESS) {
        //TODO(sean): diagnostic
        panic("Failed to allocate command buffers!");
    }

    for(usize index = 0; index < state.swapchain_image_count; index += 1) {
        VkCommandBuffer command_buffer = state.command_buffers[index];

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if(vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
            panic("Failed to begin command buffer!");
        }

        {
            const VkClearValue clear_values[] = {
                {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
                {.depthStencil = {1.0f, 0}},
            };

            VkRenderPassBeginInfo level_render_pass_begin_info = {};
            level_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            level_render_pass_begin_info.renderPass = state.level_pipeline.pass;
            level_render_pass_begin_info.framebuffer = state.swapchain_framebuffers[index];
            level_render_pass_begin_info.renderArea.offset.x = 0;
            level_render_pass_begin_info.renderArea.offset.y = 0;
            level_render_pass_begin_info.renderArea.extent.width = state.swapchain_extent.width;
            level_render_pass_begin_info.renderArea.extent.height = state.swapchain_extent.height;
            level_render_pass_begin_info.clearValueCount = 2;
            level_render_pass_begin_info.pClearValues = clear_values;

            vkCmdBeginRenderPass(command_buffer, &level_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.level_pipeline.pipeline);
                {
                    VkBuffer vertex_buffers[] = {state.level_model.vertices.buffer};
                    VkDeviceSize offsets[] = {0};
                    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
                    vkCmdBindIndexBuffer(command_buffer, state.level_model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.level_pipeline.layout, 0, 1, &state.descriptor_sets[index], 0, 0);
                }
                vkCmdDrawIndexed(command_buffer, state.level_model.index_count, 1, 0, 0, 0);
            }
            vkCmdEndRenderPass(command_buffer);

            VkRenderPassBeginInfo enemy_render_pass_begin_info = {};
            enemy_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            enemy_render_pass_begin_info.renderPass = state.enemy_pipeline.pass;
            enemy_render_pass_begin_info.framebuffer = state.swapchain_framebuffers[index];
            enemy_render_pass_begin_info.renderArea.offset.x = 0;
            enemy_render_pass_begin_info.renderArea.offset.y = 0;
            enemy_render_pass_begin_info.renderArea.extent.width = state.swapchain_extent.width;
            enemy_render_pass_begin_info.renderArea.extent.height = state.swapchain_extent.height;
            enemy_render_pass_begin_info.clearValueCount = 2;
            enemy_render_pass_begin_info.pClearValues = clear_values;

            vkCmdBeginRenderPass(command_buffer, &enemy_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.enemy_pipeline.pipeline);
                {
                    VkBuffer vertex_buffers[] = {state.enemy_model.vertices.buffer, state.enemy_position_rotation_buffer.buffer};
                    VkDeviceSize offsets[] = {0, 0};
                    vkCmdBindVertexBuffers(command_buffer, 0, 2, vertex_buffers, offsets);
                    vkCmdBindIndexBuffer(command_buffer, state.enemy_model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.enemy_pipeline.layout, 0, 1, &state.descriptor_sets[index], 0, 0);
                }
                vkCmdDrawIndexed(command_buffer, enemy_index_count, state.max_enemy_count, 0, 0, 0);
            }
            vkCmdEndRenderPass(command_buffer);
    }

        if(vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
            panic("Failed to end command buffer!");
        }
    }
}

void create_sync_objects() {
    //TODO(sean): see if we can reduce the overall amount of times we call malloc
    state.image_available_semaphores = sbmalloc(&state.semaphore_buffer, MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    state.render_finished_semaphores = sbmalloc(&state.semaphore_buffer, MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    state.in_flight_fences = sbmalloc(&state.semaphore_buffer, MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));
    state.image_in_flight_fences = sbcalloc(&state.semaphore_buffer, 0, state.swapchain_image_count * sizeof(VkFence));

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(usize index = 0; index < MAX_FRAMES_IN_FLIGHT; index += 1)
    {
        if(vkCreateSemaphore(state.device, &semaphore_info, 0, &state.image_available_semaphores[index]) != VK_SUCCESS
        || vkCreateSemaphore(state.device, &semaphore_info, 0, &state.render_finished_semaphores[index]) != VK_SUCCESS
        || vkCreateFence(state.device, &fence_info, 0, &state.in_flight_fences[index]) != VK_SUCCESS)
        {
            //TODO(sean): diagnostic
            panic("Failed to create synchronization structures for a frame!");
        }
    }
}

void create_descriptor_set_layout() {
    VkDescriptorSetLayoutBinding ubo_layout_binding = {};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = 0;

    VkDescriptorSetLayoutBinding sampler_layout_binding = {};
    sampler_layout_binding.binding = 1;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    sampler_layout_binding.pImmutableSamplers = 0;

    VkDescriptorSetLayoutBinding bindings[descriptor_count] = {ubo_layout_binding, sampler_layout_binding};
    VkDescriptorSetLayoutCreateInfo layout_create_info = {};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = 2;
    layout_create_info.pBindings = bindings;

    if(vkCreateDescriptorSetLayout(state.device, &layout_create_info, 0, &state.ubo_sampler_descriptor_set_layout) != VK_SUCCESS) {
        //TODO(sean): diagnostic
        panic("Failed to create descriptor set layout!");
    }
}

void create_uniform_buffers() {
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);

    state.uniform_buffers = sbmalloc(&state.semaphore_buffer, state.swapchain_image_count * sizeof(VkBuffer));
    state.uniform_buffers_memory = sbmalloc(&state.semaphore_buffer, state.swapchain_image_count * sizeof(VkDeviceMemory));

    for(usize index = 0; index < state.swapchain_image_count; index += 1) {
        create_buffer(
            state.device, state.physical_device, buffer_size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &state.uniform_buffers[index], &state.uniform_buffers_memory[index]
        );
    }
}

void create_descriptor_pool() {
    VkDescriptorPoolSize ubo_pool_size = {};
    ubo_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_pool_size.descriptorCount = state.swapchain_image_count;

    VkDescriptorPoolSize sampler_pool_size = {};
    sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_pool_size.descriptorCount = state.swapchain_image_count;

    VkDescriptorPoolSize pool_sizes[descriptor_count] = {ubo_pool_size, sampler_pool_size};

    VkDescriptorPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.poolSizeCount = descriptor_count;
    pool_create_info.pPoolSizes = pool_sizes;
    pool_create_info.maxSets = state.swapchain_image_count;

    if(vkCreateDescriptorPool(state.device, &pool_create_info, 0, &state.uniform_descriptor_pool) != VK_SUCCESS) {
        //TODO(sean): diagnostic
        panic("Failed to create uniform descriptor pool!");
    }
}

void create_descriptor_sets() {
    VkDescriptorSetLayout* layouts = sbmalloc(&state.swapchain_buffer, state.swapchain_image_count * sizeof(VkDescriptorSetLayout));
    for(usize index = 0; index < state.swapchain_image_count; index += 1) { layouts[index] = state.ubo_sampler_descriptor_set_layout; }

    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = state.uniform_descriptor_pool;
    allocate_info.descriptorSetCount = state.swapchain_image_count;
    allocate_info.pSetLayouts = layouts;

    state.descriptor_sets = sbmalloc(&state.swapchain_buffer, state.swapchain_image_count * sizeof(VkDescriptorSet));
    if (vkAllocateDescriptorSets(state.device, &allocate_info, state.descriptor_sets) != VK_SUCCESS) {
        //TODO(sean): diagnostic
        panic("Failed to allocate uniform descriptor sets!");
    }

    for(usize index = 0; index < state.swapchain_image_count; index += 1) {
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = state.uniform_buffers[index];
        buffer_info.offset = 0;
        buffer_info.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = state.level_texture.view;
        image_info.sampler = state.generic_sampler;

        VkWriteDescriptorSet descriptor_writes[descriptor_count] = {};
        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = state.descriptor_sets[index];
        descriptor_writes[0].dstBinding = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo = &buffer_info;
        descriptor_writes[0].pImageInfo = 0;
        descriptor_writes[0].pTexelBufferView = 0;

        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = state.descriptor_sets[index];
        descriptor_writes[1].dstBinding = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pBufferInfo = 0;
        descriptor_writes[1].pImageInfo = &image_info;
        descriptor_writes[1].pTexelBufferView = 0;

        vkUpdateDescriptorSets(state.device, descriptor_count, descriptor_writes, 0, 0);
    }
}

void cleanup_swapchain_artifacts() {
    vkDestroyImageView(state.device, state.depth_image_view, 0);
    vkDestroyImage(state.device, state.depth_image, 0);
    vkFreeMemory(state.device, state.depth_image_memory, 0);

    for(usize index = 0; index < state.swapchain_image_count; index += 1) {
        vkDestroyBuffer(state.device, state.uniform_buffers[index], 0);
        vkFreeMemory(state.device, state.uniform_buffers_memory[index], 0);
        vkDestroyFramebuffer(state.device, state.swapchain_framebuffers[index], 0);
    }

    vkDestroyDescriptorPool(state.device, state.uniform_descriptor_pool, 0);

    vkFreeCommandBuffers(state.device, state.command_pool, (u32)state.swapchain_image_count, state.command_buffers);

    destroy_pipeline(state.device, state.level_pipeline);
    destroy_pipeline(state.device, state.enemy_pipeline);

    for(usize index = 0; index < state.swapchain_image_count; index += 1) {
        vkDestroyImageView(state.device, state.swapchain_image_views[index], 0);
    }

    vkDestroySwapchainKHR(state.device, state.swapchain, 0);

    sbclear(&state.swapchain_buffer);
}

void recreate_swapchain() {
    vkDeviceWaitIdle(state.device);

    cleanup_swapchain_artifacts();

    pre_init_swapchain();
    init_swapchain();
    create_shader_modules();
    create_level_graphics_pipeline();
    create_enemy_graphics_pipeline();
    create_depth_image();
    create_swapchain_framebuffers();
    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffers();
}

void create_level_buffers() {
    create_device_local_buffer(
        state.device,
        state.physical_device,
        state.queue,
        state.command_pool,
        sizeof(Vertex) * loader->level_vertex_count,
        loader->level_vertices,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        &state.level_model.vertices.buffer,
        &state.level_model.vertices.memory
    );

    create_device_local_buffer(
        state.device,
        state.physical_device,
        state.queue,
        state.command_pool,
        sizeof(u32) * loader->level_index_count,
        loader->level_indices,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        &state.level_model.indices.buffer,
        &state.level_model.indices.memory
    );
}

void create_enemy_buffers() {
    create_device_local_buffer(
        state.device,
        state.physical_device,
        state.queue,
        state.command_pool,
        sizeof(Vertex) * enemy_vertex_count,
        enemy_vertices,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        &state.enemy_model.vertices.buffer,
        &state.enemy_model.vertices.memory
    );

    create_device_local_buffer(
        state.device,
        state.physical_device,
        state.queue,
        state.command_pool,
        sizeof(u32) * enemy_index_count,
        enemy_indices,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        &state.enemy_model.indices.buffer,
        &state.enemy_model.indices.memory
    );

    create_device_local_and_staging_buffer(
        state.device,
        state.physical_device,
        state.queue,
        state.command_pool,
        sizeof(vec4) * state.max_enemy_count,
        &state.enemy_position_rotations,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        &state.enemy_position_rotation_buffer.buffer,
        &state.enemy_position_rotation_buffer.memory,
        &state.enemy_position_rotation_staging_buffer.buffer,
        &state.enemy_position_rotation_staging_buffer.memory
    );
}

void update_time() {
    f64 now_time = glfwGetTime();
    state.delta_time = (f32)now_time - state.elapsed_time;
    state.elapsed_time = now_time;

    // Measure speed
    static double lastTime = 0.0;
    double delta = now_time - lastTime;
    static usize nbFrames = 0;
    nbFrames++;
    if(delta >= 1.0) { // If last cout was more than 1 sec ago
        double fps = (double)nbFrames / delta;

        char buffer[128];
        sprintf(buffer, "Area 52 | Version 0.1.0 | %.2lf FPS", fps);
        glfwSetWindowTitle(state.window, buffer);

        nbFrames = 0;
        lastTime = now_time;
    }
}

void init_loader() {
    loader = malloc(sizeof(LoaderState));

    sbinit(&loader->scratch, 1 * 1024 * 1024); // 1M
    sbinit(&loader->read_scratch, 2 * 1024 * 1024); // 2M
}

void load_level_texture() {
    char *texture_path = "../viking_room.png";
    loader->texture_path = sbmalloc(&loader->scratch, strlen(texture_path) * sizeof(char));
    strcpy(loader->texture_path, texture_path);

    loader->texture_pixels = stbi_load(loader->texture_path, &loader->texture_width, &loader->texture_height, &loader->texture_channels, STBI_rgb_alpha);
    loader->image_size = loader->texture_width * loader->texture_height * 4;

    if(loader->texture_pixels == 0) {
        char buffer[128];
        sprintf(buffer, "Failed to load texture image (%s)!", loader->texture_path);
        panic(buffer);
    }
}

void load_level_model() {
    loader->level_path = "../test_0.level";

    {
        FILE *fp = fopen(loader->level_path, "rb");

        fread(&loader->level_vertex_count, sizeof(u32), 1, fp);
        loader->level_vertices = sbmalloc(&loader->read_scratch, loader->level_vertex_count * sizeof(Vertex));
        for (usize index = 0; index < loader->level_vertex_count; index += 1) {
            fread(&loader->level_vertices[index].pos.x, sizeof(f32), 1, fp);
            fread(&loader->level_vertices[index].pos.y, sizeof(f32), 1, fp);
            fread(&loader->level_vertices[index].pos.z, sizeof(f32), 1, fp);

            fread(&loader->level_vertices[index].color.x, sizeof(f32), 1, fp);
            fread(&loader->level_vertices[index].color.y, sizeof(f32), 1, fp);
            fread(&loader->level_vertices[index].color.z, sizeof(f32), 1, fp);

            fread(&loader->level_vertices[index].uv.x, sizeof(f32), 1, fp);
            fread(&loader->level_vertices[index].uv.y, sizeof(f32), 1, fp);
        }

        fread(&loader->level_index_count, sizeof(u32), 1, fp);
        loader->level_indices = sbmalloc(&loader->read_scratch, loader->level_index_count * sizeof(u32));
        for (usize index = 0; index < loader->level_index_count; index += 1) {
            fread(&loader->level_indices[index], sizeof(u32), 1, fp);
        }

        fclose(fp);
    }

    //TODO(sean): load physmesh directly from level file
    {
        state.level_physmesh_vertex_count = loader->level_index_count;
        state.level_physmesh = sbmalloc(&state.physics_buffer, loader->level_index_count * sizeof(vec3));
        for(usize index = 0; index < loader->level_index_count; index += 1) {
            state.level_physmesh[index] = vec3_mul_f32(loader->level_vertices[loader->level_indices[index]].pos, 1.00);
        }
    }

    state.level_model.index_count = loader->level_index_count;
}

void free_loader() {
    stbi_image_free(loader->texture_pixels);
    sbfree(&loader->read_scratch);
    sbfree(&loader->scratch);
    free(loader);
    loader = 0;
}

int main() {
    state.window_width = 960;
    state.window_height = 540;
    state.primary_monitor = 0;

    state.theta = 1.15;
    state.phi = 0.75;
    state.mouse_sensitivity = 2.0;

    state.player_speed = 16.0;
    state.player_height = 2.0;
    state.player_radius = 1.0;
    state.gravity = 64.0;
    state.player_z_speed = 0.0f;
    state.sliding_threshold = 0.7;

    state.player_position.x = 7.0;
    state.player_position.y = 7.0;
    state.player_position.z = 7.0;

    sbinit(&state.scratch, 512 * 1024); // 512K
    sbinit(&state.swapchain_buffer, 4 * 1024); // 4K
    sbinit(&state.semaphore_buffer, 4 * 1024); // 4K

    sbinit(&state.physics_buffer, 512 * 1024); // 512K
    sbinit(&state.physics_scratch_buffer, 16 * 1024); // 16K

    init_window();
    check_layers_support();
    init_instance();
    init_surface();
    init_physical_device();
    find_depth_image_format();
    pre_init_swapchain();
    init_swapchain();
    create_shader_modules();
    create_uniform_buffers();
    create_descriptor_set_layout();
    create_level_graphics_pipeline();
    create_enemy_graphics_pipeline();
    init_command_pool();
    create_depth_image();
    create_swapchain_framebuffers();
    create_generic_sampler();
    {
        init_loader();

        load_level_texture();
        create_texture_image();

        load_level_model();
        create_vertex_buffer();
        create_index_buffer();

        free_loader();
    }
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffers();
    create_sync_objects();

    while(!glfwWindowShouldClose(state.window)) {
        update_time();
        glfwPollEvents();
        update_and_render();

        if(glfwGetKey(state.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(state.window, GLFW_TRUE);
        }

        //TODO(sean): move this kind of functionality into a struct
        static b32 f11_held = true;
        if(glfwGetKey(state.window, GLFW_KEY_F11) == GLFW_PRESS) {
            if(f11_held != true) {
                state.window_fullscreen = !state.window_fullscreen;
                if (state.window_fullscreen) {
                    state.window_width = 1920;
                    state.window_height = 1080;
                    glfwSetWindowSize(state.window, state.window_width, state.window_height);
                    glfwSetWindowPos(state.window, 0, 0);
                } else {
                    state.window_width = 800;
                    state.window_height = 600;
                    glfwSetWindowSize(state.window, state.window_width, state.window_height);
                    glfwSetWindowPos(state.window, 100, 200);
                }
            }
            f11_held = true;
        } else {
            f11_held = false;
        }
    }

    vkDeviceWaitIdle(state.device);

    return 0;
}
