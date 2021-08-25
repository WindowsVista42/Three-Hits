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
#include "alutil.h"
#include "init.h"
#include "load.h"
#include "update.h"

// lib
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

int main() {
    GameState* state = malloc(sizeof(GameState));
    memset(state, 0, sizeof(GameState));
    LoaderState* loader;

    init_staged_buffers(state);
    init_defaults(state);

    init_open_al_soft(state);
    load_al_procedures();
    init_open_al_defaults(state);

    init_window(state);
    check_layers_support(state);

    init_instance(state);
    init_surface(state);
    init_physical_device(state);
    find_depth_image_format(state);

    pre_init_swapchain(state);
    init_swapchain(state);

    create_shader_modules(state);

    create_uniform_buffers(state);
    create_descriptor_set_layout(state);

    create_level_graphics_pipeline(state);
    create_entity_graphics_pipeline(state);
    create_crosshair_graphics_pipeline(state);

    init_command_pool(state);

    create_crosshair_buffers(state);
    create_depth_image(state);
    create_swapchain_framebuffers(state);
    create_generic_sampler(state);
    create_sync_objects(state);
    alListenerf(AL_GAIN, 0.0f);

    load_level(state);
    // levels.txt
    // 0 <-- player ended index
    // level0
    // level1
    // level2

    glfwSetTime(0.0);
    while(!glfwWindowShouldClose(state->window)) {
        update_time(state);
        glfwPollEvents();
        update_and_render(state);
        //print_diagnostics();

        if(glfwGetKey(state->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(state->window, GLFW_TRUE);
        }

        //TODO(sean): move this kind of functionality into a struct
        static b32 f11_held = true;
        if(glfwGetKey(state->window, GLFW_KEY_F11) == GLFW_PRESS) {
            if(f11_held != true) {
                state->window_fullscreen = !state->window_fullscreen;
                if (state->window_fullscreen) {
                    state->window_width = 1920;
                    state->window_height = 1080;
                    glfwSetWindowSize(state->window, state->window_width, state->window_height);
                    glfwSetWindowPos(state->window, 0, 0);
                } else {
                    state->window_width = 800;
                    state->window_height = 600;
                    glfwSetWindowSize(state->window, state->window_width, state->window_height);
                    glfwSetWindowPos(state->window, 100, 200);
                }
            }
            f11_held = true;
        } else {
            f11_held = false;
        }

        if(state->load_next_level == true) {
            unload_level(state);
            load_level(state);
        }
    }

    vkDeviceWaitIdle(state->device);

    return 0;
}
