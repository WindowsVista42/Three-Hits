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

    init_command_pool(state);

    create_uniform_buffers(state);
    create_descriptor_set_layout(state);
    create_hud_data(state);

    create_all_graphics_pipelines(state);

    create_depth_image(state);
    create_swapchain_framebuffers(state);
    create_generic_sampler(state);
    create_sync_objects(state);

    load_sounds(state);
    load_level(state);
    load_actions(state, 0);
    // planned levels format
    // levels.txt
    // 0 <-- player ended index
    // level0
    // level1
    // level2

    reset_player(state);
    glfwSetTime(0.0);
    alListenerf(AL_GAIN, 1.0f);
    while(!glfwWindowShouldClose(state->window)) {
        update_time(state);
        glfwPollEvents();
        update_bind_states(state);
        update_and_render(state);
        //print_performance_statistics(state);

        if(glfwGetKey(state->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(state->window, GLFW_TRUE);
        }

        static Bind fullscreen_bind = {GLFW_KEY_F11};
        update_key_bind_state(state->window, &fullscreen_bind);
        if(fullscreen_bind.pressed) {
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

        if(state->load_next_level == true) {
            unload_level(state);
            load_level(state);
        }
    }

    vkDeviceWaitIdle(state->device);

    return 0;
}
