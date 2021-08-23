//
// Created by Windows Vista on 8/18/2021.
//
#include "init.h"

#ifndef THREE_HITS_UPDATE_H

void cleanup_swapchain_artifacts(GameState* state) {
    destroy_texture(state->device, state->depth_texture);

    for(usize index = 0; index < state->swapchain_image_count; index += 1) {
        vkDestroyBuffer(state->device, state->camera_uniforms[index].buffer, 0);
        vkFreeMemory(state->device, state->camera_uniforms[index].memory, 0);
        vkDestroyFramebuffer(state->device, state->swapchain_framebuffers[index], 0);
    }

    vkDestroyDescriptorPool(state->device, state->global_descriptor_pool, 0);

    vkFreeCommandBuffers(state->device, state->command_pool, (u32)state->swapchain_image_count, state->command_buffers);

    destroy_pipeline(state->device, state->level_pipeline);
    destroy_pipeline(state->device, state->entity_pipeline);
    destroy_pipeline(state->device, state->crosshair_pipeline);

    for(usize index = 0; index < state->swapchain_image_count; index += 1) {
        vkDestroyImageView(state->device, state->swapchain_image_views[index], 0);
    }

    vkDestroySwapchainKHR(state->device, state->swapchain, 0);

    sbclear(&state->swapchain_buffer);
}

void update_swapchain(GameState* state) {
    vkDeviceWaitIdle(state->device);

    cleanup_swapchain_artifacts(state);

    pre_init_swapchain(state);
    init_swapchain(state);
    create_shader_modules(state);
    create_level_graphics_pipeline(state);
    create_entity_graphics_pipeline(state);
    create_crosshair_graphics_pipeline(state);
    create_depth_image(state);
    create_swapchain_framebuffers(state);
    create_uniform_buffers(state);
    create_descriptor_pool(state);
    create_descriptor_sets(state);
    create_command_buffers(state);
}

void update_time(GameState* state) {
    f64 now_time = glfwGetTime();
    state->delta_time = (f32)now_time - state->elapsed_time;
    state->elapsed_time = now_time;

    // Measure speed
    static double lastTime = 0.0;
    double delta = now_time - lastTime;
    static usize nbFrames = 0;
    nbFrames++;
    if(delta >= 1.0) { // If last cout was more than 1 sec ago
        double fps = (double)nbFrames / delta;

        char buffer[128];
        sprintf(buffer, "Three Hits | Version 0.1.0 | %.2lf FPS", fps);
        glfwSetWindowTitle(state->window, buffer);

        nbFrames = 0;
        lastTime = now_time;
    }
}

void print_diagnostics(GameState* state) {
    printf("budget: %.2fms, usage: %.2fms / %.2f%%\n", (1.0 / 60.0) * 1000.0, (state->delta_time) * 1000.0, (state->delta_time / (1.0 / 60.0)) * 100.0);
}

void update(GameState* state) {
    u32 current_image = state->current_image;

    UniformBufferObject ubo = {};
    mat4 projection, view;

    b32 play_pistol_sound = false;
    b32 play_reload_sound = false;
    b32 play_movement_sound = false;
    b32 play_crouch_sound = false;
    b32 play_jump_sound = false;

    static f32 accumulated_delta_time = 0.0f;
    accumulated_delta_time += state->delta_time;
    vec3 player_eye = state->player_position;
    player_eye.z += 0.5;
    {

        //if(glfwGetKey(state->window, GLFW_KEY_R) == GLFW_PRESS) {
        //    state->player_position.x = 0.0;
        //    state->player_position.y = 0.0;
        //    state->player_position.z = 0.0;
        //    state->player_z_speed = 0.0;
        //}

        // -33.5
        // -11.5

        if(glfwGetKey(state->window, GLFW_KEY_Q) == GLFW_PRESS) {
            vec3_print("Player position: ", state->player_position);
        }

        f32 wall_distance = 4096.0;
        vec3 E = vec3_add_vec3(state->player_position, vec3_mul_f32(vec3_mul_f32(state->look_dir, -1.0), 128.0));

        for (usize index = 0; index < state->physmesh_vertex_count; index += 3) {
            vec3 A = state->physmesh_vertices[index + 0];
            vec3 B = state->physmesh_vertices[index + 1];
            vec3 C = state->physmesh_vertices[index + 2];
            vec3 P = player_eye;
            vec3 N;
            f32 d;

            // distance for closest intersection from S to E
            if (ray_intersects_triangle(A, B, C, P, E, &N, &d) == true) {
                if (d < wall_distance) {
                    wall_distance = d;
                    state->closest_ray_index = index;
                }
            }
        }

        b32 active_action = false;
        static b32 e_held = true;
        if (glfwGetKey(state->window, GLFW_KEY_E) == GLFW_PRESS) {
            if (e_held != true) {
                active_action = true;
            }
            e_held = true;
        } else {
            e_held = false;
        }

        // doors
        {
            for (usize index = 0; index < state->door_count; index += 1) {
                u32 lower = state->door_physmesh_ranges[index];
                u32 upper = state->door_physmesh_ranges[index + 1];

                // door opens
                if (lower <= state->closest_ray_index && state->closest_ray_index < upper && active_action && wall_distance < state->door_activation_distance) {
                    alSourcePlay(state->door_sound_sources[index]);
                    state->door_timings[index] = state->max_door_open_time * 2.0 - state->door_timings[index];
                    break;
                }
            }

            // door logic
            //TODO(sean): I dont really like this, but it gets the job done
            for (usize index = 0; index < state->door_count; index += 1) {
                if (state->door_timings[index] > state->max_door_open_time) {
                    state->door_position_rotations[index].z += state->delta_time * state->door_move_speed;

                    u32 lower = state->door_physmesh_ranges[index];
                    u32 upper = state->door_physmesh_ranges[index + 1];
                    for (usize phys_index = lower; phys_index < upper; phys_index += 1) {
                        state->physmesh_vertices[phys_index].z += state->delta_time * state->door_move_speed;
                    }
                } else if (state->door_timings[index] > 0.0f) {
                    state->door_position_rotations[index].z -= state->delta_time * state->door_move_speed;

                    u32 lower = state->door_physmesh_ranges[index];
                    u32 upper = state->door_physmesh_ranges[index + 1];
                    for (usize phys_index = lower; phys_index < upper; phys_index += 1) {
                        state->physmesh_vertices[phys_index].z -= state->delta_time * state->door_move_speed;
                    }
                }

                if(state->door_timings[index] > 0.0f) {
                    state->door_timings[index] -= state->delta_time;
                }
            }

            write_buffer(
                state->device,
                state->door_position_rotation_staging_buffer.memory,
                0,
                state->door_count * sizeof(vec4),
                0,
                state->door_position_rotations
            );
            copy_buffer_to_buffer(
                state->device,
                state->queue,
                state->command_pool,
                state->door_position_rotation_staging_buffer.buffer,
                state->door_position_rotation_buffer.buffer,
                state->door_count * sizeof(vec4)
            );
        }

        // player shooting
        u32 hit_index = UINT32_MAX;
        f32 best_enemy_distance = 4096.0;
        static f32 shoot_timer = 0.0f;
        static f32 reload_timer = 0.0f;
        static b32 r_held = false;

        if(reload_timer < 0.0f && glfwGetKey(state->window, GLFW_KEY_R) == GLFW_PRESS) {
            if(r_held == false) {
                play_reload_sound = true;
                reload_timer = state->pistol_reload_speed;
                state->loaded_pistol_ammo_count = state->pistol_magazine_size;
            } else {
                r_held = true;
            }
        } else {
            r_held = false;
        }

        if (glfwGetMouseButton(state->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if(shoot_timer < 0.0f && reload_timer < 0.0f) {
                shoot_timer = state->pistol_shoot_delay;
                play_pistol_sound = true;
                state->loaded_pistol_ammo_count -= 1;

                if(state->loaded_pistol_ammo_count == 0) {
                    play_reload_sound = true;
                    reload_timer = state->pistol_reload_speed;
                    state->loaded_pistol_ammo_count = state->pistol_magazine_size;
                }

                for (usize index = 0; index < state->enemy_alive_count; index += 1) {
                    vec3 Pe = *(vec3 *) &state->enemy_position_rotations[index].x;
                    vec3 P = player_eye;
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
        }
        if(reload_timer > 0.0f) { state->crosshair_color = vec4_new(0.0, 0.0, 1.0, 0.5); }
        else if(shoot_timer > 0.0f) { state->crosshair_color = vec4_new(1.0, 0.0, 0.0, 0.5); }
        else { state->crosshair_color = vec4_new(1.0, 1.0, 1.0, 0.5); }
        write_buffer(state->device, state->crosshair_color_staging_buffer.memory, 0, sizeof(vec4), 0, &state->crosshair_color);
        copy_buffer_to_buffer(
            state->device,
            state->queue,
            state->command_pool,
            state->crosshair_color_staging_buffer.buffer,
            state->crosshair_color_buffer.buffer,
            sizeof(vec4)
        );
        shoot_timer -= state->delta_time;
        reload_timer -= state->delta_time;

        const vec4 death_flash_color = vec4_new(0.0, 0.0, 1.0, 0.0);
        const vec4 damage_flash_color = vec4_new(1.0, 0.0, 0.0, 1.0);
        const vec4 default_color = vec4_new(0.0, 1.0, 0.0, 1.0);

        // enemy hit
        if (hit_index != UINT32_MAX) {
            alSourcePlay(state->enemy_alert_sound_sources[hit_index]);

            if(state->enemy_healths[hit_index] > 0) { state->enemy_healths[hit_index] -= 1; }
            if(state->enemy_healths[hit_index] <= 0) { // we just killed it
                // swap with last one
                usize back_index = state->enemy_alive_count - 1;
                i32 temp_health = state->enemy_healths[hit_index];
                vec4 _temp_color = vec4_copy(state->enemy_colors[hit_index]);
                vec4 temp_position_rotation = vec4_copy(state->enemy_position_rotations[hit_index]);

                state->enemy_healths[hit_index] = state->enemy_healths[back_index];
                state->enemy_colors[hit_index] = state->enemy_colors[back_index];
                state->enemy_position_rotations[hit_index] = state->enemy_position_rotations[back_index];

                state->enemy_healths[back_index] = temp_health;
                state->enemy_colors[back_index] = death_flash_color;
                state->enemy_position_rotations[back_index] = temp_position_rotation;

                state->enemy_alive_count -= 1;

                alSourceStop(state->enemy_alert_sound_sources[back_index]);
                alSourceStop(state->enemy_ambience_sound_sources[back_index]);
                alSourceStop(state->enemy_windup_sound_sources[hit_index]);
                alSourcei(state->enemy_alert_sound_sources[back_index], AL_BUFFER, state->enemy_explosion_sound_buffer);
                alSourcefv(state->enemy_alert_sound_sources[back_index], AL_POSITION, (f32*)&state->enemy_position_rotations[back_index]);
                alSourcef(state->enemy_alert_sound_sources[back_index], AL_GAIN, 1.0f);
                alSourcef(state->enemy_alert_sound_sources[back_index], AL_ROLLOFF_FACTOR, 0.8f);
                alSourcePlay(state->enemy_alert_sound_sources[back_index]);
            } else {
                state->enemy_colors[hit_index] = damage_flash_color;
                state->enemy_hit_times[hit_index] = 0.125;
            }
        } else {
            for(usize index = 0; index < state->enemy_alive_count; index += 1) {
                if(state->enemy_hit_times[index] < 0.0) {
                    state->enemy_colors[index] = default_color;
                }
                state->enemy_hit_times[index] -= state->delta_time;
            }
        }

        for(usize index = 0; index < state->max_enemy_count; index += 1) {
            state->enemy_position_rotations[index].w = atan2f(
                state->player_position.x - state->enemy_position_rotations[index].x,
                state->player_position.y - state->enemy_position_rotations[index].y
            );
        }

        u64 position_rotation_copy_size = state->max_enemy_count * sizeof(vec4);
        write_buffer(state->device, state->enemy_position_rotation_staging_buffer.memory, 0, position_rotation_copy_size, 0, state->enemy_position_rotations);
        copy_buffer_to_buffer(
            state->device,
            state->queue,
            state->command_pool,
            state->enemy_position_rotation_staging_buffer.buffer,
            state->enemy_position_rotation_buffer.buffer,
            position_rotation_copy_size
        );

        u64 color_copy_size = state->max_enemy_count * sizeof(vec4);
        write_buffer(state->device, state->enemy_color_staging_buffer.memory, 0, color_copy_size, 0, state->enemy_colors);
        copy_buffer_to_buffer(
            state->device,
            state->queue,
            state->command_pool,
            state->enemy_color_staging_buffer.buffer,
            state->enemy_color_buffer.buffer,
            color_copy_size
        );
    }

    b32 space_pressed = false;
    static b32 space_held = false;
    static b32 airborne = true;
    vec3 xydir = {{state->look_dir.x, state->look_dir.y, 0.0}};
    xydir = vec3_norm(xydir);
    vec3 normal = {{-xydir.y, xydir.x, 0.0}};
    vec3 delta_movement = VEC3_ZERO;
    f32 player_speed = state->player_speed;
    f32 player_radius = state->player_radius;
    {
        if(glfwGetKey(state->window, GLFW_KEY_W) == GLFW_PRESS) {
            delta_movement = vec3_sub_vec3(delta_movement, xydir);
            play_movement_sound = true;
        }
        if(glfwGetKey(state->window, GLFW_KEY_A) == GLFW_PRESS) {
            delta_movement = vec3_sub_vec3(delta_movement, normal);
            play_movement_sound = true;
        }
        if(glfwGetKey(state->window, GLFW_KEY_S) == GLFW_PRESS) {
            delta_movement = vec3_add_vec3(delta_movement, xydir);
            play_movement_sound = true;
        }
        if(glfwGetKey(state->window, GLFW_KEY_D) == GLFW_PRESS) {
            delta_movement = vec3_add_vec3(delta_movement, normal);
            play_movement_sound = true;
        }
        if(glfwGetKey(state->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            player_speed /= 2.0;
        } else if(glfwGetKey(state->window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            player_speed /= 2.0;
            player_radius /= 2.0;
            play_crouch_sound = true;
        }


        if(glfwGetKey(state->window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            space_pressed = true;
            play_jump_sound = true;
        }

        state->theta = f32_wrap(state->theta, 2.0 * M_PI);

        state->look_dir = vec3_from_theta_phi(state->theta, state->phi);
    }

    {
        static b32 play_enemy_windup[30] = {};
        f32 delta_time = state->delta_time / 4.0;
        // Enemy physics
        for(usize i = 0; i < 4; i += 1) {
            for (usize index = 0; index < state->enemy_alive_count; index += 1) {
                vec3 P = *(vec3 *) &state->enemy_position_rotations[index];
                f32 rr = 1.0;
                vec3 N;
                f32 d;

                f32 sr = state->enemy_simulation_radius;
                f32 srsr = sr * sr;
                if (vec3_distsq_vec3(P, state->player_position) > srsr) { continue; }

                f32 move_speed = 3.0;
                vec3 PN = vec3_norm(vec3_sub_vec3(*(vec3 *) &state->enemy_position_rotations[index], state->player_position));
                if(vec3_distsq_vec3(P, state->player_position) > rr + state->player_radius + 1.0) {
                    *(vec3 *) &state->enemy_position_rotations[index] = vec3_add_vec3(*(vec3 *) &state->enemy_position_rotations[index], vec3_mul_f32(PN, delta_time * -move_speed));
                }

                // Check if we are colliding with any of our kind
                for (usize phys_index = 0; phys_index < state->enemy_alive_count; phys_index += 1) {
                    if (phys_index == index) { continue; }
                    vec3 p = *(vec3 *) &state->enemy_position_rotations[phys_index];
                    N = vec3_norm(vec3_sub_vec3(P, p));
                    f32 distsq = vec3_distsq_vec3(P, p);

                    if (distsq < rr + rr) {
                        d = sqrtf((rr + rr) - distsq);
                        *(vec3 *) &state->enemy_position_rotations[index] = vec3_add_vec3(*(vec3 *) &state->enemy_position_rotations[index], vec3_mul_f32(N, d));
                    }
                }

                u32 vertex_count = 0;
                vec3 *vertices = sbmalloc(&state->physics_scratch_buffer, 1200 * sizeof(vec3));

                // cull
                for (usize phys_index = 0; phys_index < state->physmesh_vertex_count; phys_index += 3) {
                    vec3 A = state->physmesh_vertices[phys_index + 0];
                    vec3 B = state->physmesh_vertices[phys_index + 1];
                    vec3 C = state->physmesh_vertices[phys_index + 2];

                    // intersection culling
                    vec3 ABC = vec3_div_f32(vec3_add_vec3(vec3_add_vec3(A, B), C), 3.0);
                    f32 ABCrr = vec3_distsq_vec3(ABC, A) + 2.0;
                    f32 ABCBrr = vec3_distsq_vec3(ABC, B) + 2.0;
                    f32 ABCCrr = vec3_distsq_vec3(ABC, C) + 2.0;
                    if (ABCBrr > ABCrr) { ABCrr = ABCBrr; }
                    if (ABCCrr > ABCrr) { ABCrr = ABCCrr; }
                    if (vec3_distsq_vec3(ABC, P) < ABCrr + rr) {
                        vertices[vertex_count + 0] = A;
                        vertices[vertex_count + 1] = B;
                        vertices[vertex_count + 2] = C;
                        vertex_count += 3;
                    }
                }

                f32 wall_distance = 4096.0;
                vec3 E = state->player_position;
                b32 found_wall = false;
                for (usize phys_index = 0; phys_index < state->physmesh_vertex_count; phys_index += 3) {
                    vec3 A = state->physmesh_vertices[phys_index + 0];
                    vec3 B = state->physmesh_vertices[phys_index + 1];
                    vec3 C = state->physmesh_vertices[phys_index + 2];
                    vec3 rN;
                    f32 rd;

                    f32 pd = vec3_distsq_vec3(P, E);

                    // distance for closest intersection from S to E
                    if (ray_intersects_triangle(A, B, C, P, E, &rN, &rd) == true) {
                        if(rd < pd) {
                            found_wall = true;
                            break;
                        }
                    }
                }

                ALenum source_state;
                // player shot logic
                b32 sees_player = false;
                if(found_wall == false) { // can see the player
                    sees_player = true;
                    alGetSourcei(state->enemy_windup_sound_sources[index], AL_SOURCE_STATE, &source_state);
                    if(source_state == AL_PLAYING && state->windup_needs_reverse[index] == true) {
                        // if windup sound already playing, reverse windup sound | so it starts playing forwards again
                        //alSourceRewind(state->enemy_windup_sound_sources[index]);
                        // stop old and play rewinded at right spot
                        ALfloat offset;
                        alGetSourcef(state->enemy_windup_sound_sources[index], AL_SEC_OFFSET, &offset);
                        ALfloat remainder = state->enemy_shoot_delay - offset;
                        alSourceStop(state->enemy_windup_sound_sources[index]);
                        alSourcei(state->enemy_windup_sound_sources[index], AL_BUFFER, state->enemy_windup_sound_buffer);
                        alSourcef(state->enemy_windup_sound_sources[index], AL_SEC_OFFSET, remainder);
                        alSourcePlay(state->enemy_windup_sound_sources[index]);

                        state->windup_needs_reverse[index] = false;
                    } else {
                        // else play windup sound once
                        if(source_state != AL_PLAYING) {
                            alSourcei(state->enemy_windup_sound_sources[index], AL_BUFFER, state->enemy_windup_sound_buffer);
                            alSourcePlay(state->enemy_windup_sound_sources[index]);
                        }
                        state->windup_needs_reverse[index] = false;
                    }

                    state->enemy_shoot_times[index] += delta_time;
                    if(state->enemy_shoot_times[index] > state->enemy_shoot_delay) { // can shoot the player
                        // stop the windup sound
                        // play shoot sound
                        state->enemy_shoot_times[index] = 0.0f;
                        alSourcePlay(state->enemy_gun_sound_sources[index]);
                        alSourceStop(state->enemy_windup_sound_sources[index]);
                        state->player_health -= 1;
                    }
                } else { // can not see the player
                    alGetSourcei(state->enemy_windup_sound_sources[index], AL_SOURCE_STATE, &source_state);
                    if(source_state == AL_PLAYING && state->windup_needs_reverse[index] == true) {
                        // if windup sound is playing then reverse the windup sound once | so it starts playing backwards again
                        ALfloat offset;
                        alGetSourcef(state->enemy_windup_sound_sources[index], AL_SEC_OFFSET, &offset);
                        ALfloat remainder = state->enemy_shoot_delay - offset;
                        alSourceStop(state->enemy_windup_sound_sources[index]);
                        alSourcei(state->enemy_windup_sound_sources[index], AL_BUFFER, state->enemy_reverse_windup_sound_buffer);
                        alSourcef(state->enemy_windup_sound_sources[index], AL_SEC_OFFSET, remainder);
                        alSourcePlay(state->enemy_windup_sound_sources[index]);

                        state->windup_needs_reverse[index] = false;
                    }

                    state->enemy_shoot_times[index] -= delta_time;
                    if(state->enemy_shoot_times[index] < 0.0f) {
                        state->enemy_shoot_times[index] = 0.0f;
                    }
                }

                if(state->enemy_sees_player[index] != sees_player) {
                    state->windup_needs_reverse[index] = true;
                }
                state->enemy_sees_player[index] = sees_player;

                // Check if we are colliding with any of the triangles in the physmesh
                for (usize phys_index = 0; phys_index < vertex_count; phys_index += 3) {
                    vec3 A = vertices[phys_index + 0];
                    vec3 B = vertices[phys_index + 1];
                    vec3 C = vertices[phys_index + 2];

                    if (sphere_collides_with_triangle(A, B, C, P, rr, &N, &d)) {
                        f32 sliding_factor = vec3_dot(N, VEC3_UNIT_Z);

                        if (sliding_factor > state->sliding_threshold) { N = VEC3_UNIT_Z; }
                        if (vec3_eq_vec3(N, VEC3_ZERO)) { N = VEC3_UNIT_Z; }

                        *(vec3*)&state->enemy_position_rotations[index] = vec3_add_vec3(*(vec3*)&state->enemy_position_rotations[index], vec3_mul_f32(N, d));
                    }
                }

                sbclear(&state->physics_scratch_buffer);
            }
        }
    }

    if(state->player_health <= 0) {
        panic("Died!");
    }

    // Player physics
    {
        u32 vertex_count = 0;
        vec3 *vertices = sbmalloc(&state->physics_scratch_buffer, 1200 * sizeof(vec3));

        f32 r = player_radius;
        f32 rr = r * r;
        vec3 N;
        f32 d;

        for (usize index = 0; index < state->physmesh_vertex_count; index += 3) {
            vec3 A = state->physmesh_vertices[index + 0];
            vec3 B = state->physmesh_vertices[index + 1];
            vec3 C = state->physmesh_vertices[index + 2];
            vec3 P = state->player_position;

            // player intersection culling
            vec3 ABC = vec3_div_f32(vec3_add_vec3(vec3_add_vec3(A, B), C), 3.0);
            f32 ABCrr = vec3_distsq_vec3(ABC, A) + 2.0;
            f32 ABCBrr = vec3_distsq_vec3(ABC, B) + 2.0;
            f32 ABCCrr = vec3_distsq_vec3(ABC, C) + 2.0;
            if (ABCBrr > ABCrr) { ABCrr = ABCBrr; }
            if (ABCCrr > ABCrr) { ABCrr = ABCCrr; }
            if (vec3_distsq_vec3(ABC, P) < ABCrr + rr) {
                vertices[vertex_count + 0] = A;
                vertices[vertex_count + 1] = B;
                vertices[vertex_count + 2] = C;
                vertex_count += 3;
            }
        }

        //TODO(sean): more optimizations can technically be made here, such as only running the required number of steps given our framerate/deltatime
        f32 delta_time = state->delta_time / 12.0;
        for (usize i = 0; i < 12; i += 1) {
            if (space_pressed && !space_held && airborne) {
                state->player_z_speed = -state->player_jump_speed;
                state->player_position.z -= state->player_z_speed * delta_time;
                space_held = true;
                airborne = false;
            } else {
                state->player_position.z -= state->player_z_speed * delta_time;
                space_held = false;
            }

            if (vec3_par_ne_vec3(delta_movement, VEC3_ZERO)) {
                delta_movement = vec3_norm(delta_movement);
                delta_movement = vec3_mul_f32(delta_movement, delta_time);
                delta_movement = vec3_mul_f32(delta_movement, player_speed);
                state->player_position = vec3_add_vec3(state->player_position, delta_movement);
            }

            state->player_z_speed += state->gravity * delta_time;
            {
                static b32 hit_head = false;

                // Check if we are colliding with any of the triangles in the physmesh
                for (usize index = 0; index < vertex_count; index += 3) {
                    vec3 A = vertices[index + 0];
                    vec3 B = vertices[index + 1];
                    vec3 C = vertices[index + 2];
                    vec3 P = state->player_position;

                    if (sphere_collides_with_triangle(A, B, C, P, rr, &N, &d)) {
                        d += (1.0 / 512.0);
                        f32 sliding_factor = vec3_dot(N, VEC3_UNIT_Z);

                        if (sliding_factor > state->sliding_threshold) {
                            N = VEC3_UNIT_Z;
                            state->player_z_speed = state->gravity * state->slide_gravity_factor * delta_time;
                            airborne = true;
                            hit_head = false;
                        } else if (sliding_factor < -state->sliding_threshold && !airborne && !hit_head) {
                            state->player_z_speed = 0.0f;
                            hit_head = true;
                        }
                        if (vec3_eq_vec3(N, VEC3_ZERO)) { N = VEC3_UNIT_Z; }
                        state->player_position = vec3_add_vec3(state->player_position, vec3_mul_f32(N, d));
                    }
                }
            }

            {
                // Check if we are colliding with any enemies
                for(usize index = 0; index < state->enemy_alive_count; index += 1) {
                    vec3 P = state->player_position;
                    vec3 p = *(vec3*)&state->enemy_position_rotations[index];
                    N = vec3_norm(vec3_sub_vec3(P, p));
                    f32 distsq = vec3_distsq_vec3(P, p);

                    if(distsq < rr + 1.0) {
                        d = sqrtf((rr + 1.0f) - distsq);
                        state->player_position = vec3_add_vec3(state->player_position, vec3_mul_f32(N, d));
                    }
                }
            }
        }

        sbclear(&state->physics_scratch_buffer);
    }

    view = mat4_look_dir(player_eye, state->look_dir, VEC3_UNIT_Z);
    projection = mat4_perspective(f32_radians(100.0f), (float)state->swapchain_extent.width / (float)state->swapchain_extent.height, 0.01f, 1000.0f);
    ubo.view_projection = mat4_mul_mat4(view, projection);
    write_buffer(state->device, state->camera_uniforms[current_image].memory, 0, sizeof(ubo), 0, &ubo);

    {
        ALenum source_state;

        alGetSourcei(state->player_movement_sound_source, AL_SOURCE_STATE, &source_state);
        if(play_movement_sound == true && source_state != AL_PLAYING) {
            alSourcei(state->player_movement_sound_source, AL_BUFFER, state->player_movement_sound_buffer);
            alSourcePlay(state->player_movement_sound_source);
        } else if (airborne == false) {
            alSourceStop(state->player_movement_sound_source);
        }

        alSourcefv(state->player_movement_sound_source, AL_POSITION, (f32*)&player_eye);

        if (play_pistol_sound == true) { alSourcePlay(state->player_gun_sound_source); }
        alSourcefv(state->player_gun_sound_source, AL_POSITION, (f32*)&player_eye);

        for(usize index = 0; index < state->enemy_alive_count; index += 1) {
            alSourcefv(state->enemy_alert_sound_sources[index], AL_POSITION, (f32*)&state->enemy_position_rotations[index]);
        }

        for(usize index = 0; index < state->enemy_alive_count; index += 1) {
            alSourcefv(state->enemy_ambience_sound_sources[index], AL_POSITION, (f32*)&state->enemy_position_rotations[index]);
        }

        for(usize index = 0; index < state->enemy_alive_count; index += 1) {
            alSourcefv(state->enemy_gun_sound_sources[index], AL_POSITION, (f32*)&state->enemy_position_rotations[index]);
        }

        for(usize index = 0; index < state->enemy_alive_count; index += 1) {
            alSourcefv(state->enemy_windup_sound_sources[index], AL_POSITION, (f32*)&state->enemy_position_rotations[index]);
        }

        for(usize index = 0; index < state->door_count; index += 1) {
            alSourcefv(state->door_sound_sources[index], AL_POSITION, (f32*)&state->door_position_rotations[index]);
        }

        f32 orientation[6] = {-state->look_dir.x, -state->look_dir.y, -state->look_dir.z, 0.0f, 0.0f, 1.0f};
        alListenerfv(AL_ORIENTATION, orientation);
        alListenerfv(AL_POSITION,  (f32*)&player_eye);
    }
}

void update_and_render(GameState* state) {
    vkWaitForFences(state->device, 1, &state->in_flight_fences[state->current_frame_index], VK_TRUE, UINT64_MAX);

    u32 image_index;
    VkResult result;

    result = vkAcquireNextImageKHR(state->device, state->swapchain, UINT64_MAX, state->image_available_semaphores[state->current_frame_index], 0, &image_index);

    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        update_swapchain(state);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        panic("Failed to acquire next swapchain image!");
    }

    state->current_image = image_index;
    update(state);

    if(state->image_in_flight_fences[state->current_frame_index] != 0) {
        vkWaitForFences(state->device, 1, &state->image_in_flight_fences[state->current_frame_index], VK_TRUE, UINT64_MAX);
    }
    state->image_in_flight_fences[state->current_frame_index] = state->in_flight_fences[state->current_frame_index];

    VkSemaphore wait_semaphores[] = {state->image_available_semaphores[state->current_frame_index]};
    VkSemaphore signal_semaphores[] = {state->render_finished_semaphores[state->current_frame_index]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &state->command_buffers[image_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkResetFences(state->device, 1, &state->in_flight_fences[state->current_frame_index]);

    if(vkQueueSubmit(state->queue, 1, &submit_info, state->in_flight_fences[state->current_frame_index]) != VK_SUCCESS) {
        panic("Failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapchains[] = {state->swapchain};
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = 0;

    result = vkQueuePresentKHR(state->present_queue, &present_info);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        update_swapchain(state);
    } else if (result != VK_SUCCESS) {
        panic("Failed to present swapchain image!");
    }

    state->current_frame_index += 1;
    state->current_frame_index %= MAX_FRAMES_IN_FLIGHT;
}

#define THREE_HITS_UPDATE_H

#endif //THREE_HITS_UPDATE_H
