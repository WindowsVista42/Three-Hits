//
// Created by Windows Vista on 8/18/2021.
//
#include "init.h"
#include "input.h"

#ifndef THREE_HITS_UPDATE_H

void update_bind_states(GameState* state) {
    update_mouse_bind_state(state->window, &state->shoot_button);

    update_key_bind_state(state->window, &state->activate_key);
    update_key_bind_state(state->window, &state->reload_key);
    update_key_bind_state(state->window, &state->forward_key);
    update_key_bind_state(state->window, &state->backward_key);
    update_key_bind_state(state->window, &state->left_key);
    update_key_bind_state(state->window, &state->right_key);
    update_key_bind_state(state->window, &state->walk_key);
    update_key_bind_state(state->window, &state->crouch_key);
    update_key_bind_state(state->window, &state->jump_key);

    update_key_bind_state(state->window, &state->debug_xp_key);
    update_key_bind_state(state->window, &state->debug_xn_key);
    update_key_bind_state(state->window, &state->debug_yp_key);
    update_key_bind_state(state->window, &state->debug_yn_key);
    update_key_bind_state(state->window, &state->debug_zp_key);
    update_key_bind_state(state->window, &state->debug_zn_key);
    update_key_bind_state(state->window, &state->debug_wp_key);
    update_key_bind_state(state->window, &state->debug_wn_key);
    update_key_bind_state(state->window, &state->debug_next_key);
    update_key_bind_state(state->window, &state->debug_mode_key);
}

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

void print_performance_statistics(GameState* state) {
    static f32 timer = 0.0f;
    static u32 frame_count = 0;
    static f32 low = FLT_MAX;
    static f32 high = 0.0;

    const u32 target = 60;
    const f32 threshold = 1.0;

    frame_count += 1;
    timer += state->delta_time;

    if(state->delta_time > high) { high = state->delta_time; }
    if(state->delta_time < low) { low = state->delta_time; }

    if(timer > threshold) {
        //TODO(sean): fix this so that the threshold doesn't have to be 1 for this to work
        printf(
            "-----Performance Statistics-----\n"
            "Target:  %.2fms (%.2f%%)\n"
            "Average: %.2fms (%.2f%%)\n"
            "High:    %.2fms (%.2f%%)\n"
            "Low:     %.2fms (%.2f%%)\n"
            "\n",
            (1.0 / (f32)target) * 1000.0, 100.0,
            (1.0 / (f32)frame_count) * 1000.0, 100.0 / ((f32)frame_count / (f32)target),
            (f32)high * 1000.0, 100.0 * ((f32)high / (1.0 / (f32)target)),
            (f32)low * 1000.0, 100.0 * ((f32)low / (1.0 / (f32)target))
        );

        timer -= threshold;
        frame_count = 0;
        low = FLT_MAX;
        high = 0.0;
    }
}

void sync_entity_position_rotations(
    VkDevice device,
    VkQueue queue,
    VkCommandPool command_pool,
    vec3 player_position,
    EntityList* entities
) {
    for(usize index = 0; index < entities->capacity; index += 1) {
        entities->position_rotations[index].w = atan2f(
            player_position.x - entities->position_rotations[index].x,
            player_position.y - entities->position_rotations[index].y
        );
    }

    u64 position_rotation_copy_size = entities->capacity * sizeof(vec4);
    write_buffer(device, entities->position_rotation_staging_buffer.memory, 0, position_rotation_copy_size, 0, entities->position_rotations);
    copy_buffer_to_buffer(
        device,
        queue,
        command_pool,
        entities->position_rotation_staging_buffer.buffer,
        entities->position_rotation_buffer.buffer,
        position_rotation_copy_size
    );

    u64 color_copy_size = entities->capacity * sizeof(vec4);
    write_buffer(device, entities->color_staging_buffer.memory, 0, color_copy_size, 0, entities->colors);
    copy_buffer_to_buffer(
        device,
        queue,
        command_pool,
        entities->color_staging_buffer.buffer,
        entities->color_buffer.buffer,
        color_copy_size
    );
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
        /*
        if(glfwGetKey(state->window, GLFW_KEY_Q) == GLFW_PRESS) {
            vec3_print("Player position: ", state->player_position);
        }
        */

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

        // doors
        {
            for (usize index = 0; index < state->doors.capacity; index += 1) {
                u32 lower = state->door_physmesh_ranges[index];
                u32 upper = state->door_physmesh_ranges[index + 1];

                // door opens
                if (lower <= state->closest_ray_index && state->closest_ray_index < upper && state->activate_key.pressed && wall_distance < state->door_activation_distance) {
                    if(state->door_requirements[index] == 0 || state->door_requirements[index] & state->player_keycards) {
                        alSourcePlay(state->door_sound_sources[index]);
                        state->door_timings[index] = state->max_door_open_time * 2.0 - state->door_timings[index];
                        break;
                    } else {
                        //TODO(sean): switch
                        printf("Cannot open door you don't have the right keycard\n");
                        printf("Door requires %d\n", state->door_requirements[index]);
                    }
                }
            }

            // door logic
            //TODO(sean): I dont really like this, but it gets the job done
            for each(usize, index, 0, state->doors.capacity) {
            //for (usize index = 0; index < state->doors.capacity; index += 1) {
                if (state->door_timings[index] > state->max_door_open_time) {
                    state->doors.position_rotations[index].z += state->delta_time * state->door_move_speed;

                    u32 lower = state->door_physmesh_ranges[index];
                    u32 upper = state->door_physmesh_ranges[index + 1];
                    for (usize phys_index = lower; phys_index < upper; phys_index += 1) {
                        state->physmesh_vertices[phys_index].z += state->delta_time * state->door_move_speed;
                    }
                } else if (state->door_timings[index] > 0.0f) {
                    state->doors.position_rotations[index].z -= state->delta_time * state->door_move_speed;

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
                state->doors.position_rotation_staging_buffer.memory,
                0,
                state->doors.capacity * sizeof(vec4),
                0,
                state->doors.position_rotations
            );
            copy_buffer_to_buffer(
                state->device,
                state->queue,
                state->command_pool,
                state->doors.position_rotation_staging_buffer.buffer,
                state->doors.position_rotation_buffer.buffer,
                state->doors.capacity * sizeof(vec4)
            );
        }

        // player shooting
        u32 hit_index = UINT32_MAX;
        f32 best_enemy_distance = 4096.0;
        static f32 shoot_timer = 0.0f;
        static f32 reload_timer = 0.0f;

        if(reload_timer < 0.0f && state->reload_key.pressed) {
            play_reload_sound = true;
            reload_timer = state->pistol_reload_speed;
            state->loaded_pistol_ammo_count = state->pistol_magazine_size;
        }

        if(state->shoot_button.held) {
            if(shoot_timer < 0.0f && reload_timer < 0.0f) {
                shoot_timer = state->pistol_shoot_delay;
                play_pistol_sound = true;
                state->loaded_pistol_ammo_count -= 1;

                if(state->loaded_pistol_ammo_count == 0) {
                    play_reload_sound = true;
                    reload_timer = state->pistol_reload_speed;
                    state->loaded_pistol_ammo_count = state->pistol_magazine_size;
                }

                for (usize index = 0; index < state->mediums.entities.length; index += 1) {
                    vec3 Pe = *(vec3 *) &state->mediums.entities.position_rotations[index].x;
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

        write_buffer_copy_buffer(
            state->device,
            state->queue,
            state->command_pool,
            state->crosshair_color_staging_buffer,
            state->crosshair_color_buffer,
            &state->crosshair_color,
            0,
            sizeof(vec4),
            0
        );

        shoot_timer -= state->delta_time;
        reload_timer -= state->delta_time;

        const vec4 death_flash_color = vec4_new(0.0, 0.0, 1.0, 0.0);
        const vec4 damage_flash_color = vec4_new(1.0, 0.0, 0.0, 1.0);
        const vec4 default_color = vec4_new(0.0, 1.0, 0.0, 1.0);

        // enemy hit
        if (hit_index != UINT32_MAX) {
            alSourcePlay(state->mediums.alert_sound_sources[hit_index]);

            if(state->mediums.healths[hit_index] > 0) {
                state->mediums.healths[hit_index] -= 1;
            }
            if(state->mediums.healths[hit_index] <= 0) { // we just killed it
                // swap with last one
                usize back_index = state->mediums.entities.length - 1;
                i32 temp_health = state->mediums.healths[hit_index];
                vec4 _temp_color = vec4_copy(state->mediums.entities.colors[hit_index]);
                vec4 temp_position_rotation = vec4_copy(state->mediums.entities.position_rotations[hit_index]);

                state->mediums.healths[hit_index] = state->mediums.healths[back_index];
                state->mediums.entities.colors[hit_index] = state->mediums.entities.colors[back_index];
                state->mediums.entities.position_rotations[hit_index] = state->mediums.entities.position_rotations[back_index];

                state->mediums.healths[back_index] = temp_health;
                state->mediums.entities.colors[back_index] = death_flash_color;
                state->mediums.entities.position_rotations[back_index] = temp_position_rotation;

                state->mediums.entities.length -= 1;

                alSourceStop(state->mediums.alert_sound_sources[back_index]);
                alSourceStop(state->mediums.ambience_sound_sources[back_index]);
                alSourceStop(state->mediums.windup_sound_sources[hit_index]);
                alSourcei(state->mediums.alert_sound_sources[back_index], AL_BUFFER, state->medium_sounds.explosion);
                alSourcefv(state->mediums.alert_sound_sources[back_index], AL_POSITION, (f32*)&state->mediums.entities.position_rotations[back_index]);
                alSourcef(state->mediums.alert_sound_sources[back_index], AL_GAIN, 1.0f);
                alSourcef(state->mediums.alert_sound_sources[back_index], AL_ROLLOFF_FACTOR, 0.8f);
                alSourcePlay(state->mediums.alert_sound_sources[back_index]);
            } else {
                state->mediums.entities.colors[hit_index] = damage_flash_color;
                state->mediums.hit_times[hit_index] = state->mediums.hit_reaction_duration;
            }
        } else {
            for(usize index = 0; index < state->mediums.entities.length; index += 1) {
                if(state->mediums.hit_times[index] < 0.0) {
                    state->mediums.entities.colors[index] = default_color;
                }
                state->mediums.hit_times[index] -= state->delta_time;
            }
        }

        sync_entity_position_rotations(state->device, state->queue, state->command_pool, state->player_position, &state->mediums.entities);
        sync_entity_position_rotations(state->device, state->queue, state->command_pool, state->player_position, &state->rats.entities);
        sync_entity_position_rotations(state->device, state->queue, state->command_pool, state->player_position, &state->knights.entities);
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
        if(state->forward_key.held) {
            delta_movement = vec3_sub_vec3(delta_movement, xydir);
            play_movement_sound = true;
        }
        if(state->left_key.held) {
            delta_movement = vec3_sub_vec3(delta_movement, normal);
            play_movement_sound = true;
        }
        if(state->backward_key.held) {
            delta_movement = vec3_add_vec3(delta_movement, xydir);
            play_movement_sound = true;
        }
        if(state->right_key.held) {
            delta_movement = vec3_add_vec3(delta_movement, normal);
            play_movement_sound = true;
        }
        if(state->walk_key.held) {
            player_speed /= 2.0;
        } else if(state->crouch_key.held) {
            player_speed /= 2.0;
            player_radius /= 2.0;
            play_crouch_sound = true;
        }

        if(state->jump_key.held) {
            space_pressed = true;
            play_jump_sound = true;
        }

        state->theta = f32_wrap(state->theta, 2.0 * M_PI);

        state->look_dir = vec3_from_theta_phi(state->theta, state->phi);
    }

    {
        static b32 play_enemy_windup[30] = {};
        f32 delta_time = state->delta_time / 4.0;
        // enemy physics
        for(usize i = 0; i < 4; i += 1) {
            for (usize index = 0; index < state->mediums.entities.length; index += 1) {
                vec3 P = *(vec3 *) &state->mediums.entities.position_rotations[index];
                f32 rr = 1.0;
                vec3 N;
                f32 d;

                f32 sr = state->mediums.activation_range;
                f32 srsr = sr * sr;
                if (vec3_distsq_vec3(P, state->player_position) > srsr) { continue; }

                f32 move_speed = 3.0;
                vec3 PN = vec3_norm(vec3_sub_vec3(*(vec3 *) &state->mediums.entities.position_rotations[index], state->player_position));
                if (vec3_distsq_vec3(P, state->player_position) > rr + (state->player_radius * state->player_radius) + 4.0) {
                    *(vec3 *) &state->mediums.entities.position_rotations[index] = vec3_add_vec3(*(vec3 *) &state->mediums.entities.position_rotations[index], vec3_mul_f32(PN, delta_time * -move_speed));
                } else {
                    *(vec3 *) &state->mediums.entities.position_rotations[index] = vec3_add_vec3(*(vec3 *) &state->mediums.entities.position_rotations[index], vec3_mul_f32(PN, delta_time * move_speed));
                }

                // Check if we are colliding with any of our kind
                for (usize phys_index = 0; phys_index < state->mediums.entities.length; phys_index += 1) {
                    if (phys_index == index) { continue; }
                    vec3 p = *(vec3 *) &state->mediums.entities.position_rotations[phys_index];
                    N = vec3_norm(vec3_sub_vec3(P, p));
                    f32 distsq = vec3_distsq_vec3(P, p);

                    if (distsq < rr + rr) {
                        d = sqrtf((rr + rr) - distsq);
                        *(vec3 *) &state->mediums.entities.position_rotations[index] = vec3_add_vec3(*(vec3 *) &state->mediums.entities.position_rotations[index], vec3_mul_f32(N, d));
                    }
                }

                u32 vertex_count = 0;
                vec3 *vertices = sbmalloc(&state->level_scratch_buffer, 1200 * sizeof(vec3));

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
                    alGetSourcei(state->mediums.windup_sound_sources[index], AL_SOURCE_STATE, &source_state);
                    if(source_state == AL_PLAYING && state->mediums.reverse_windup[index] == true) {
                        // if windup sound already playing, reverse windup sound | so it starts playing forwards again
                        //alSourceRewind(state->enemy_windup_sound_sources[index]);
                        // stop old and play rewinded at right spot
                        ALfloat offset;
                        alGetSourcef(state->mediums.windup_sound_sources[index], AL_SEC_OFFSET, &offset);
                        ALfloat remainder = state->mediums.shoot_delay - offset;
                        alSourceStop(state->mediums.windup_sound_sources[index]);
                        alSourcei(state->mediums.windup_sound_sources[index], AL_BUFFER, state->medium_sounds.windup);
                        alSourcef(state->mediums.windup_sound_sources[index], AL_SEC_OFFSET, remainder);
                        alSourcePlay(state->mediums.windup_sound_sources[index]);

                        state->mediums.reverse_windup[index] = false;
                    } else {
                        // else play windup sound once
                        if(source_state != AL_PLAYING) {
                            alSourcei(state->mediums.windup_sound_sources[index], AL_BUFFER, state->medium_sounds.windup);
                            alSourcePlay(state->mediums.windup_sound_sources[index]);
                        }
                        state->mediums.reverse_windup[index] = false;
                    }

                    state->mediums.shoot_times[index] += delta_time;
                    if(state->mediums.shoot_times[index] > state->mediums.shoot_delay) { // can shoot the player
                        // stop the windup sound
                        // play shoot sound
                        state->mediums.shoot_times[index] = 0.0f;
                        alSourcePlay(state->mediums.gun_sound_sources[index]);
                        alSourceStop(state->mediums.windup_sound_sources[index]);
                        state->player_health -= 1;
                    }
                } else { // can not see the player
                    alGetSourcei(state->mediums.windup_sound_sources[index], AL_SOURCE_STATE, &source_state);
                    if(source_state == AL_PLAYING && state->mediums.reverse_windup[index] == true) {
                        // if windup sound is playing then reverse the windup sound once | so it starts playing backwards again
                        ALfloat offset;
                        alGetSourcef(state->mediums.windup_sound_sources[index], AL_SEC_OFFSET, &offset);
                        ALfloat remainder = state->mediums.shoot_delay - offset;
                        alSourceStop(state->mediums.windup_sound_sources[index]);
                        alSourcei(state->mediums.windup_sound_sources[index], AL_BUFFER, state->medium_sounds.winddown);
                        alSourcef(state->mediums.windup_sound_sources[index], AL_SEC_OFFSET, remainder);
                        alSourcePlay(state->mediums.windup_sound_sources[index]);

                        state->mediums.reverse_windup[index] = false;
                    }

                    state->mediums.shoot_times[index] -= delta_time;
                    if(state->mediums.shoot_times[index] < 0.0f) {
                        state->mediums.shoot_times[index] = 0.0f;
                    }
                }

                if(state->mediums.sees_player[index] != sees_player) {
                    state->mediums.reverse_windup[index] = true;
                }
                state->mediums.sees_player[index] = sees_player;

                // Check if we are colliding with any of the triangles in the physmesh
                for (usize phys_index = 0; phys_index < vertex_count; phys_index += 3) {
                    vec3 A = vertices[phys_index + 0];
                    vec3 B = vertices[phys_index + 1];
                    vec3 C = vertices[phys_index + 2];

                    if (sphere_collides_with_triangle(A, B, C, P, rr, &N, &d)) {
                        f32 sliding_factor = vec3_dot(N, VEC3_UNIT_Z);

                        if (sliding_factor > state->sliding_threshold) { N = VEC3_UNIT_Z; }
                        if (vec3_eq_vec3(N, VEC3_ZERO)) { N = VEC3_UNIT_Z; }

                        *(vec3*)&state->mediums.entities.position_rotations[index] = vec3_add_vec3(*(vec3*)&state->mediums.entities.position_rotations[index], vec3_mul_f32(N, d));
                    }
                }

                sbclear(&state->level_scratch_buffer);
            }
        }
    }

    if(state->player_health <= 0) {
        panic("Died!");
    }

    // Player physics
    {
        u32 vertex_count = 0;
        vec3 *vertices = sbmalloc(&state->level_scratch_buffer, 1200 * sizeof(vec3));

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
                for(usize index = 0; index < state->mediums.entities.length; index += 1) {
                    vec3 P = state->player_position;
                    vec3 p = *(vec3*)&state->mediums.entities.position_rotations[index];
                    N = vec3_norm(vec3_sub_vec3(P, p));
                    f32 distsq = vec3_distsq_vec3(P, p);

                    if(distsq < rr + 1.0) {
                        d = sqrtf((rr + 1.0f) - distsq);
                        state->player_position = vec3_add_vec3(state->player_position, vec3_mul_f32(N, d));
                    }
                }
            }
        }

        sbclear(&state->level_scratch_buffer);
    }

    view = mat4_look_dir(player_eye, state->look_dir, VEC3_UNIT_Z);
    projection = mat4_perspective(f32_radians(100.0f), (float)state->swapchain_extent.width / (float)state->swapchain_extent.height, 0.01f, 1000.0f);
    ubo.view_projection = mat4_mul_mat4(view, projection);
    write_buffer(state->device, state->camera_uniforms[current_image].memory, 0, sizeof(ubo), 0, &ubo);

    // keycard logic
    {
        for (usize index = 0; index < state->keycards.length; index += 1) {
            vec3 P = *(vec3 *) &state->keycards.position_rotations[index];
            if (vec3_distsq_vec3(P, state->player_position) < 1.0 + (state->player_radius * state->player_radius)) {
                printf("Picked up keycard %llu\n", index);
                state->keycards.position_rotations[index] = vec4_new(FLT_MAX, FLT_MAX, FLT_MAX, 0.0);
                state->player_keycards |= 1 << index;
            }
        }

        sync_entity_position_rotations(state->device, state->queue, state->command_pool, state->player_position, &state->keycards);
    }

    // sync audio buffers
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

        for(usize index = 0; index < state->mediums.entities.length; index += 1) {
            alSourcefv(state->mediums.alert_sound_sources[index], AL_POSITION, (f32*)&state->mediums.entities.position_rotations[index]);
        }

        for(usize index = 0; index < state->mediums.entities.length; index += 1) {
            alSourcefv(state->mediums.ambience_sound_sources[index], AL_POSITION, (f32*)&state->mediums.entities.position_rotations[index]);
        }

        for(usize index = 0; index < state->mediums.entities.length; index += 1) {
            alSourcefv(state->mediums.gun_sound_sources[index], AL_POSITION, (f32*)&state->mediums.entities.position_rotations[index]);
        }

        for(usize index = 0; index < state->mediums.entities.length; index += 1) {
            alSourcefv(state->mediums.windup_sound_sources[index], AL_POSITION, (f32*)&state->mediums.entities.position_rotations[index]);
        }

        for(usize index = 0; index < state->doors.capacity; index += 1) {
            alSourcefv(state->door_sound_sources[index], AL_POSITION, (f32*)&state->doors.position_rotations[index]);
        }

        f32 orientation[6] = {-state->look_dir.x, -state->look_dir.y, -state->look_dir.z, 0.0f, 0.0f, 1.0f};
        alListenerfv(AL_ORIENTATION, orientation);
        alListenerfv(AL_POSITION,  (f32*)&player_eye);
    }

    // check player position end thing
    {
        if(vec3_distsq_vec3(state->player_position, *(vec3*)&state->end_zone) < state->end_zone.w) {
            if(state->activate_key.pressed) {
                state->load_next_level = true;
            }
        }

        /*
        if(glfwGetKey(state->window, GLFW_KEY_J) == GLFW_PRESS) {
            state->load_next_level = true;
        }
        */
    }

    {
        b32 changed = false;
        static u32 mode = 0;
        static u32 index = 0;

        if(state->debug_next.pressed) {
            f32 smallest_distance = 4096.0f;
            u32 closest_index = 0;
            for(usize i = 0; i < state->ulight_count; i += 1) {
                vec3 P = *(vec3*)&state->lights[i].position_falloff;
                f32 dist = vec3_distsq_vec3(state->player_position, P);
                if(dist < smallest_distance) {
                    smallest_distance = dist;
                    closest_index = i;
                }
            }
            index = closest_index;
            printf("Changed to light %d\n", index);
        }
        if(state->debug_mode.pressed) {
            mode += 1;
            mode %= 2;
            printf("Changed to placement mode %d\n", mode);
        }

        if(mode == 0) {
            if (state->debug_xp.held) {
                changed = true;
                state->lights[index].position_falloff.x += 2.5 * state->delta_time;
            }
            if (state->debug_xn.held) {
                changed = true;
                state->lights[index].position_falloff.x -= 2.5 * state->delta_time;
            }
            if (state->debug_yp.held) {
                changed = true;
                state->lights[index].position_falloff.y += 2.5 * state->delta_time;
            }
            if (state->debug_yn.held) {
                changed = true;
                state->lights[index].position_falloff.y -= 2.5 * state->delta_time;
            }
            if (state->debug_zp.held) {
                changed = true;
                state->lights[index].position_falloff.z += 2.5 * state->delta_time;
            }
            if (state->debug_zn.held) {
                changed = true;
                state->lights[index].position_falloff.z -= 2.5 * state->delta_time;
            }
            if (state->debug_wp.held) {
                changed = true;
                state->lights[index].position_falloff.w += 0.5 * state->delta_time;
            }
            if (state->debug_wn.held) {
                changed = true;
                state->lights[index].position_falloff.w -= 0.5 * state->delta_time;
            }

            if(changed) {
                vec4_print(state->lights[index].position_falloff);
            }
        } else if(mode == 1) {
            if (state->debug_xp.held) {
                changed = true;
                state->lights[index].color_alpha.x += 1.0 * state->delta_time;
            }
            if (state->debug_xn.held) {
                changed = true;
                state->lights[index].color_alpha.x -= 1.0 * state->delta_time;
            }
            if (state->debug_yp.held) {
                changed = true;
                state->lights[index].color_alpha.y += 1.0 * state->delta_time;
            }
            if (state->debug_yn.held) {
                changed = true;
                state->lights[index].color_alpha.y -= 1.0 * state->delta_time;
            }
            if (state->debug_zp.held) {
                changed = true;
                state->lights[index].color_alpha.z += 1.0 * state->delta_time;
            }
            if (state->debug_zn.held) {
                changed = true;
                state->lights[index].color_alpha.z -= 1.0 * state->delta_time;
            }

            if(changed) {
                vec4_print(state->lights[index].color_alpha);
            }
        }

        //*(vec3*)&state->lights[0].position_falloff = state->player_position;
        //state->lights[0].position_falloff.w = -0.015;
        //state->lights[0].color_alpha.x = -0.5;

        if(changed) {
            write_buffer_copy_buffer(
                state->device,
                state->queue,
                state->command_pool,
                state->light_staging_buffer,
                state->light_buffer,
                state->lights,
                0,
                state->ulight_count * sizeof(Light),
                0
            );
        }
    }
}

void update_and_render(GameState* state) {
    vkWaitForFences(state->device, 1, &state->in_flight_fences[state->current_frame_index], VK_TRUE, UINT64_MAX);

    VkResult result;

    result = vkAcquireNextImageKHR(state->device, state->swapchain, UINT64_MAX, state->image_available_semaphores[state->current_frame_index], 0, &state->current_image);

    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        update_swapchain(state);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        panic("Failed to acquire next swapchain image!");
    }

    {
        update(state);
    }

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
    submit_info.pCommandBuffers = &state->command_buffers[state->current_image];
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
    present_info.pImageIndices = &state->current_image;
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
