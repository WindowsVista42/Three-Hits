//
// Created by Windows Vista on 8/18/2021.
//

#ifndef THREE_HITS_LOAD_H
#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

#include "alutil.h"

void load_level_sounds(GameState* state, LoaderState* loader) {
    {
        ReverbProperties reverb = EFX_REVERB_PRESET_BATHROOM;
        state->reverb = reverb;
    }
    alGenEffects(1, &state->reverb_effect);
    alEffecti(state->reverb_effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

    alEffectf(state->reverb_effect, AL_EAXREVERB_DENSITY, state->reverb.flDensity);
    alEffectf(state->reverb_effect, AL_EAXREVERB_DIFFUSION, state->reverb.flDiffusion);
    alEffectf(state->reverb_effect, AL_EAXREVERB_GAIN, state->reverb.flGain);
    alEffectf(state->reverb_effect, AL_EAXREVERB_GAINHF, state->reverb.flGainHF);
    alEffectf(state->reverb_effect, AL_EAXREVERB_GAINLF, state->reverb.flGainLF);
    alEffectf(state->reverb_effect, AL_EAXREVERB_DECAY_TIME, 0.2);
    alEffectf(state->reverb_effect, AL_EAXREVERB_DECAY_HFRATIO, state->reverb.flDecayHFRatio);
    alEffectf(state->reverb_effect, AL_EAXREVERB_DECAY_LFRATIO, state->reverb.flDecayLFRatio);
    alEffectf(state->reverb_effect, AL_EAXREVERB_REFLECTIONS_GAIN, state->reverb.flReflectionsGain);
    alEffectf(state->reverb_effect, AL_EAXREVERB_REFLECTIONS_DELAY, state->reverb.flReflectionsDelay);
    alEffectfv(state->reverb_effect, AL_EAXREVERB_REFLECTIONS_PAN, state->reverb.flReflectionsPan);
    alEffectf(state->reverb_effect, AL_EAXREVERB_LATE_REVERB_GAIN, state->reverb.flLateReverbGain);
    alEffectf(state->reverb_effect, AL_EAXREVERB_LATE_REVERB_DELAY, state->reverb.flLateReverbDelay);
    alEffectfv(state->reverb_effect, AL_EAXREVERB_LATE_REVERB_PAN, state->reverb.flLateReverbPan);
    alEffectf(state->reverb_effect, AL_EAXREVERB_ECHO_TIME, state->reverb.flEchoTime);
    alEffectf(state->reverb_effect, AL_EAXREVERB_ECHO_DEPTH, state->reverb.flEchoDepth);
    alEffectf(state->reverb_effect, AL_EAXREVERB_MODULATION_TIME, state->reverb.flModulationTime);
    alEffectf(state->reverb_effect, AL_EAXREVERB_MODULATION_DEPTH, state->reverb.flModulationDepth);
    alEffectf(state->reverb_effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, state->reverb.flAirAbsorptionGainHF);
    alEffectf(state->reverb_effect, AL_EAXREVERB_HFREFERENCE, state->reverb.flHFReference);
    alEffectf(state->reverb_effect, AL_EAXREVERB_LFREFERENCE, state->reverb.flLFReference);
    alEffectf(state->reverb_effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, state->reverb.flRoomRolloffFactor);
    alEffecti(state->reverb_effect, AL_EAXREVERB_DECAY_HFLIMIT, state->reverb.iDecayHFLimit);

    alGenAuxiliaryEffectSlots(1, &state->reverb_slot);
    alAuxiliaryEffectSloti(state->reverb_slot, AL_EFFECTSLOT_EFFECT, (ALint)state->reverb_effect);

    load_sound(&loader->read_scratch, "../data/sounds/pistol_sound_2.wav", &state->pistol_sound_buffer);
    load_sound(&loader->read_scratch, "../data/sounds/enemy_ambient_sound.wav", &state->enemy_ambience_sound_buffer);
    load_sound(&loader->read_scratch, "../data/sounds/enemy_alert_sound.wav", &state->enemy_alert_sound_buffer);
    load_sound(&loader->read_scratch, "../data/sounds/enemy_explosion_sound.wav", &state->enemy_explosion_sound_buffer);
    load_sound(&loader->read_scratch, "../data/sounds/rifle_sound.wav", &state->enemy_gun_sound_buffer);
    load_sound(&loader->read_scratch, "../data/sounds/enemy_windup_sound.wav", &state->enemy_windup_sound_buffer);
    load_sound(&loader->read_scratch, "../data/sounds/enemy_reverse_windup_sound.wav", &state->enemy_reverse_windup_sound_buffer);
    load_sound(&loader->read_scratch, "../data/sounds/door_opening_sound.wav", &state->door_opening_sound_buffer);
    load_sound(&loader->read_scratch, "../data/sounds/footstep_sound.wav", &state->player_movement_sound_buffer);
    load_sound(&loader->read_scratch, "../data/sounds/footstep_sound.wav", &state->player_jump_sound_buffer);

    generate_sound_source(&state->player_gun_sound_source, 0.9f, AL_FALSE, state->pistol_sound_buffer, state->reverb_slot);
    generate_sound_source(&state->player_movement_sound_source, 0.2f, AL_FALSE, state->player_movement_sound_buffer, state->reverb_slot);
    generate_sound_sources(&state->level_buffer, &state->enemy_alert_sound_sources, state->enemies.capacity, 0.8f, AL_FALSE, state->enemy_alert_sound_buffer, state->reverb_slot);

    generate_sound_sources(&state->level_buffer, &state->enemy_gun_sound_sources, state->enemies.capacity, 1.0f, AL_FALSE, state->enemy_gun_sound_buffer, state->reverb_slot);
    set_sources_f(state->enemy_gun_sound_sources, state->enemies.capacity, AL_ROLLOFF_FACTOR, 0.2);

    generate_sound_sources(&state->level_buffer, &state->enemy_windup_sound_sources, state->enemies.capacity, 0.5f, AL_FALSE, state->enemy_windup_sound_buffer, state->reverb_slot);
    set_sources_f(state->enemy_windup_sound_sources, state->enemies.capacity, AL_ROLLOFF_FACTOR, 0.8);

    generate_sound_sources(&state->level_buffer, &state->door_sound_sources, state->doors.capacity, 1.2f, AL_FALSE, state->door_opening_sound_buffer, state->reverb_slot);

    generate_sound_sources(&state->level_buffer, &state->enemy_ambience_sound_sources, state->enemies.capacity, 0.8f, AL_TRUE, state->enemy_ambience_sound_buffer, state->reverb_slot);
    set_sources_f(state->enemy_ambience_sound_sources, state->enemies.capacity, AL_ROLLOFF_FACTOR, 3.0);
    play_sources(state->enemy_ambience_sound_sources, state->enemies.length);
}

void load_level_texture(GameState* state, LoaderState* loader) {
    char *texture_path = "../data/textures/doors0.png";
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

void read_vertices(StagedBuffer* scratch_buffer, u32* vertex_count, Vertex** vertices, FILE* fp) {
    fread(vertex_count, sizeof(u32), 1, fp);
    *vertices = sbmalloc(scratch_buffer, *vertex_count * sizeof(Vertex));
    for (usize index = 0; index < *vertex_count; index += 1) {
        fread(&(*vertices)[index].position.x, sizeof(f32), 1, fp);
        fread(&(*vertices)[index].position.y, sizeof(f32), 1, fp);
        fread(&(*vertices)[index].position.z, sizeof(f32), 1, fp);

        fread(&(*vertices)[index].normal.x, sizeof(f32), 1, fp);
        fread(&(*vertices)[index].normal.y, sizeof(f32), 1, fp);
        fread(&(*vertices)[index].normal.z, sizeof(f32), 1, fp);

        fread(&(*vertices)[index].uv.x, sizeof(f32), 1, fp);
        fread(&(*vertices)[index].uv.y, sizeof(f32), 1, fp);
    }
}

void read_indices(StagedBuffer* scratch_buffer, u32* index_count, u32** indices, FILE* fp) {
    fread(index_count, sizeof(u32), 1, fp);
    *indices = sbmalloc(scratch_buffer, *index_count * sizeof(u32));
    for (usize index = 0; index < *index_count; index += 1) {
        fread(&(*indices)[index], sizeof(u32), 1, fp);
    }
}

//TODO(sean): figure out a way to compress this
void load_level_model(GameState* state, LoaderState* loader) {
    loader->level_path = "../data/levels/doors0.level";
    if(state->level_index > 0) {
        loader->level_path = "../data/levels/doors1.level";
    }

    {
        FILE *fp = fopen(loader->level_path, "rb");
        panic_if_zero(fp, "Failed to open level!");

        // load level model
        read_vertices(&loader->read_scratch, &loader->level_vertex_count, &loader->level_vertices, fp);
        read_indices(&loader->read_scratch, &loader->level_index_count, &loader->level_indices, fp);
        state->level_model.index_count = loader->level_index_count;

        read_vertices(&loader->read_scratch, &loader->door_vertex_count, &loader->door_vertices, fp);
        read_indices(&loader->read_scratch, &loader->door_index_count, &loader->door_indices, fp);
        state->doors.model.index_count = loader->door_index_count;

        // load physmesh
        fread(&state->physmesh_vertex_count, sizeof(u32), 1, fp);
        state->physmesh_vertices = sbmalloc(&state->level_buffer, state->physmesh_vertex_count * sizeof(vec3));
        for (usize index = 0; index < state->physmesh_vertex_count; index += 1) {
            fread(&state->physmesh_vertices[index].x, sizeof(f32), 1, fp);
            fread(&state->physmesh_vertices[index].y, sizeof(f32), 1, fp);
            fread(&state->physmesh_vertices[index].z, sizeof(f32), 1, fp);
        }

        // load door prs
        fread(&state->doors.capacity, sizeof(u32), 1, fp);
        state->doors.position_rotations = sbmalloc(&state->level_buffer, state->doors.capacity * sizeof(vec4));
        for (usize index = 0; index < state->doors.capacity; index += 1) {
            fread(&state->doors.position_rotations[index].x, sizeof(f32), 1, fp);
            fread(&state->doors.position_rotations[index].y, sizeof(f32), 1, fp);
            fread(&state->doors.position_rotations[index].z, sizeof(f32), 1, fp);
            fread(&state->doors.position_rotations[index].w, sizeof(f32), 1, fp);
        }

        // load doors physmesh ranges
        fread(&state->door_physmesh_range_count, sizeof(u32), 1, fp);
        state->door_physmesh_ranges = sbmalloc(&state->level_buffer, state->door_physmesh_range_count * sizeof(u32));
        for (usize index = 0; index < state->door_physmesh_range_count; index += 1) {
            fread(&state->door_physmesh_ranges[index], sizeof(u32), 1, fp);
        }

        // set door defaults
        state->level_model.index_count = loader->level_index_count;
        state->door_timings = sbcalloc(&state->level_buffer, 0, state->doors.capacity * sizeof(f32));
        state->doors.colors = sbmalloc(&state->level_buffer, state->doors.capacity * sizeof(vec4));
        for(usize index = 0; index < state->doors.capacity; index += 1) {
            state->doors.colors[index] = vec4_new(1.0, 1.0, 0.0, 1.0);
        }

        /*
        // load enemies from file
        fread(&state->enemies.capacity, sizeof(u32), 1, fp);
        fread(&state->enemies.length, sizeof(u32), 1, fp);
        state->enemies.position_rotations = sbmalloc(&state->level_buffer, state->enemies.capacity * sizeof(vec4));
        for (usize index = 0; index < state->enemies.length; index += 1) {
            fread(&state->enemies.position_rotations[index].x, sizeof(f32), 1, fp);
            fread(&state->enemies.position_rotations[index].y, sizeof(f32), 1, fp);
            fread(&state->enemies.position_rotations[index].z, sizeof(f32), 1, fp);
            state->enemies.position_rotations[index].w = 0.0f;
        }

        for(usize index = state->enemies.length; index < state->enemies.capacity; index += 1) {
            state->enemies.position_rotations[index].x = FLT_MAX;
            state->enemies.position_rotations[index].y = FLT_MAX;
            state->enemies.position_rotations[index].z = FLT_MAX;
            state->enemies.position_rotations[index].w = 0.0f;
        }

        // set enemy defaults
        state->enemies.colors = sbmalloc(&state->level_buffer, state->enemies.capacity * sizeof(vec4));
        for(u32 index = 0; index < state->enemies.capacity; index += 1) {
            state->enemies.colors[index].x = 1.0;
            state->enemies.colors[index].y = 0.0;
            state->enemies.colors[index].z = 0.0;
            state->enemies.colors[index].w = 1.0;
        }

        const i32 enemy_default_health = 4;
        state->enemy_healths = sbmalloc(&state->level_buffer, state->enemies.capacity * sizeof(i32));
        for(u32 index = 0; index < state->enemies.capacity; index += 1) {
            state->enemy_healths[index] = enemy_default_health;
        }

        state->enemy_hit_times = sbcalloc(&state->level_buffer, 0, state->enemies.capacity * sizeof(f32));
        state->enemy_shoot_times = sbcalloc(&state->level_buffer, 0, state->enemies.capacity * sizeof(f32));
        state->enemy_sees_player = sbmalloc(&state->level_buffer, state->enemies.capacity * sizeof(b32));
        state->windup_needs_reverse = sbmalloc(&state->level_buffer, state->enemies.capacity * sizeof(b32));

        // load lights from file
        fread(&loader->ulight_count, sizeof(u32), 1, fp);
        loader->lights = sbmalloc(&loader->read_scratch, loader->ulight_count * sizeof(Light));
        for(usize index = 0; index < loader->ulight_count; index += 1) {
            fread(&loader->lights[index].color_alpha.x, sizeof(f32), 1, fp);
            fread(&loader->lights[index].color_alpha.y, sizeof(f32), 1, fp);
            fread(&loader->lights[index].color_alpha.z, sizeof(f32), 1, fp);
            fread(&loader->lights[index].color_alpha.w, sizeof(f32), 1, fp);

            fread(&loader->lights[index].position_falloff.x, sizeof(f32), 1, fp);
            fread(&loader->lights[index].position_falloff.y, sizeof(f32), 1, fp);
            fread(&loader->lights[index].position_falloff.z, sizeof(f32), 1, fp);
            fread(&loader->lights[index].position_falloff.w, sizeof(f32), 1, fp);
        }
        */

        fclose(fp);
    }

    // load enemies
    {
        state->enemies.capacity = 30;
        state->enemies.length = 0;//state->enemies.capacity;
        u32 i = 0;

        state->enemies.position_rotations = sbmalloc(&state->level_buffer, state->enemies.capacity * sizeof(vec4));
        state->enemy_hit_times = sbmalloc(&state->level_buffer, state->enemies.capacity * sizeof(f32));
        state->enemies.colors = sbmalloc(&state->level_buffer, state->enemies.capacity * sizeof(vec4));
        state->enemy_healths = sbmalloc(&state->level_buffer, state->enemies.capacity * sizeof(i32));
        state->enemy_shoot_times = sbcalloc(&state->level_buffer, 0, state->enemies.capacity * sizeof(f32));

        // starting room
        state->enemies.position_rotations[i++] = vec4_new(-4.5, -20.0, 1.0, 0.0);

        // right hallway
        state->enemies.position_rotations[i++] = vec4_new(-23.5, -31.0, 1.0, 0.0);

        // right room
        state->enemies.position_rotations[i++] = vec4_new(-26.0, -16.0, 1.0, 0.0);
        state->enemies.position_rotations[i++] = vec4_new(-24.0, -8.0 , 1.0, 0.0);
        state->enemies.position_rotations[i++] = vec4_new(-32.5,  0.25, 1.0, 0.0);
        state->enemies.position_rotations[i++] = vec4_new(-37.0,  0.25, 1.0, 0.0);
        state->enemies.position_rotations[i++] = vec4_new(-43.0, -9.5 , 1.0, 0.0);

        // upper hallway
        state->enemies.position_rotations[i++] = vec4_new(-5.25, -58.5, 10.5, 0.0);
        state->enemies.position_rotations[i++] = vec4_new( 5.5,  -58.5, 10.5, 0.0);
        state->enemies.position_rotations[i++] = vec4_new(18.0,  -64.0, 10.5, 0.0);

        // upper left room
        state->enemies.position_rotations[i++] = vec4_new(20.5,  -73.0, 10.5, 0.0);
        state->enemies.position_rotations[i++] = vec4_new(15.5,  -72.5, 10.5, 0.0);
        state->enemies.position_rotations[i++] = vec4_new(29.0,  -72.0, 10.5, 0.0);

        state->enemies.position_rotations[i++] = vec4_new(25.25, -88.0, 10.5, 0.0);
        state->enemies.position_rotations[i++] = vec4_new(26.0,  -83.0, 10.5, 0.0);
        state->enemies.position_rotations[i++] = vec4_new( 9.0,  -74.5, 10.5, 0.0);
        state->enemies.position_rotations[i++] = vec4_new( 9.0,  -78.5, 10.5, 0.0);

        // upper right room
        state->enemies.position_rotations[i++] = vec4_new(-17.5, -64.5, 10.5, 0.0);
        state->enemies.position_rotations[i++] = vec4_new(-16.5, -75.0, 10.5, 0.0);
        state->enemies.position_rotations[i++] = vec4_new(-25.5, -65.5, 10.5, 0.0);
        state->enemies.position_rotations[i++] = vec4_new(-30.0, -71.5, 10.5, 0.0);

        // bottom of stairs
        state->enemies.position_rotations[i++] = vec4_new( 9.5,  -54.5, -10.5, 0.0);

        // rest
        state->enemies.length = i;
        for(u32 index = state->enemies.length; index < state->enemies.capacity; index += 1) {
            state->enemies.position_rotations[index].x = 100.0 + (f32)i;
            state->enemies.position_rotations[index].y = 100.0 + (f32)i;
            state->enemies.position_rotations[index].z = 1.0f;
            state->enemies.position_rotations[index].w = 0.0f;
        }

        // other init data
        for(u32 index = 0; index < state->enemies.length; index += 1) {
            state->enemies.colors[index].x = 1.0;
            state->enemies.colors[index].y = 0.0;
            state->enemies.colors[index].z = 0.0;
        }

        const i32 enemy_default_health = 4;
        for(u32 index = 0; index < state->enemies.length; index += 1) {
            state->enemy_healths[index] = enemy_default_health;
        }

        for(u32 index = 0; index < state->enemies.length; index += 1) {
            state->enemy_hit_times[index] = 0.0;
        }
    }

    {
        state->enemy_sees_player = sbmalloc(&state->level_buffer, state->enemies.capacity * sizeof(b32));
        state->windup_needs_reverse = sbmalloc(&state->level_buffer, state->enemies.capacity * sizeof(b32));
    }

    // load lights
    {
        Light lights[light_count] = {
            {{{-4.5, -21.5, 0.0, sqrtf(1.0/16.0)}}, {{0.1, 0.1, 4.0, 1.0}}},
            {{{-33.0, -9.0, 0.0, sqrtf(1.0/2.0)}}, {{4.0, 0.1, 0.1, 1.0}}},
            {{{-34.5, -64.5, 10.0, sqrtf(1.0/4.0)}}, {{4.0, 1.0, 0.1, 1.0}}},
            {{{21.5, -77.5, 10.0, sqrtf(1.0/2.0)}}, {{4.0, 2.0, 0.1, 1.0}}},
            {{{9.5, -48.5, -9.5, sqrtf(1.0/1.0)}}, {{0.1, 4.0, 0.1, 1.0}}},
            {{{0.5, -56.0, 10.0, sqrtf(1.0/1.0)}}, {{4.0, 4.0, 0.1, 1.0}}},
        };

        create_device_local_buffer_2(
            state->device,
            state->physical_device,
            state->queue,
            state->command_pool,
            light_count * sizeof(Light),
            lights,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            &state->light_buffer
        );
    }

    // load keycards
    {
        state->keycards.capacity = 3;
        state->keycards.length = 3;
        state->keycards.position_rotations = sbmalloc(&state->level_buffer, state->keycards.capacity * sizeof(vec4));
        state->keycards.position_rotations[0] = vec4_new(-33.0, -9.0, 0.0, 0.0);
        state->keycards.position_rotations[1] = vec4_new(21.5, -77.5, 10.0, 0.0);
        state->keycards.position_rotations[2] = vec4_new(-34.5, -64.5, 10.0, 0.0);

        state->keycards.colors = sbmalloc(&state->level_buffer, state->keycards.capacity * sizeof(vec4));
    }

    // set door keycard requirements
    {
        state->door_requirements = sbmalloc(&state->level_buffer, state->doors.capacity * sizeof(u32));

        state->door_requirements[0] = KEYCARD_NONE;
        state->door_requirements[6] = KEYCARD_NONE;
        state->door_requirements[2] = KEYCARD_NONE;

        state->door_requirements[3] = KEYCARD_RED;
        state->door_requirements[5] = KEYCARD_RED;

        state->door_requirements[4] = KEYCARD_BLUE;

        state->door_requirements[1] = KEYCARD_YELLOW;
    }
}

typedef struct RenderEntityListInfo {
    VkCommandBuffer command_buffer;
    VkRenderPassBeginInfo* begin_info;
    VkSubpassContents subpass_contents;
    Pipeline pipeline;
    VkDescriptorSet* descriptor_set;
} RenderEntityListInfo ;

void render_entity_list(
    RenderEntityListInfo* info,
    EntityList* entities
) {
    vkCmdBeginRenderPass(info->command_buffer, info->begin_info, info->subpass_contents);
    {
        vkCmdBindPipeline(info->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info->pipeline.pipeline);
        {
            VkBuffer vertex_buffers[] = {entities->model.vertices.buffer, entities->position_rotation_buffer.buffer, entities->color_buffer.buffer};
            VkDeviceSize offsets[] = {0, 0, 0};
            vkCmdBindVertexBuffers(info->command_buffer, 0, 3, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(info->command_buffer, entities->model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(info->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info->pipeline.layout, 0, 1, info->descriptor_set, 0, 0);
        }
        vkCmdDrawIndexed(info->command_buffer, entities->model.index_count, entities->capacity, 0, 0, 0);
    }
    vkCmdEndRenderPass(info->command_buffer);
}

void create_command_buffers(GameState* state) {
    state->command_buffers = sbmalloc(&state->level_buffer, state->swapchain_image_count * sizeof(VkCommandBuffer));

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = state->command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = (u32)state->swapchain_image_count;

    if(vkAllocateCommandBuffers(state->device, &command_buffer_allocate_info, state->command_buffers) != VK_SUCCESS) {
        panic("Failed to allocate command buffers!");
    }

    for(usize index = 0; index < state->swapchain_image_count; index += 1) {
        VkCommandBuffer command_buffer = state->command_buffers[index];

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
            level_render_pass_begin_info.renderPass = state->level_pipeline.pass;
            level_render_pass_begin_info.framebuffer = state->swapchain_framebuffers[index];
            level_render_pass_begin_info.renderArea.offset.x = 0;
            level_render_pass_begin_info.renderArea.offset.y = 0;
            level_render_pass_begin_info.renderArea.extent.width = state->swapchain_extent.width;
            level_render_pass_begin_info.renderArea.extent.height = state->swapchain_extent.height;
            level_render_pass_begin_info.clearValueCount = 2;
            level_render_pass_begin_info.pClearValues = clear_values;

            vkCmdBeginRenderPass(command_buffer, &level_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->level_pipeline.pipeline);
                {
                    VkBuffer vertex_buffers[] = {state->level_model.vertices.buffer};
                    VkDeviceSize offsets[] = {0};
                    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
                    vkCmdBindIndexBuffer(command_buffer, state->level_model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->level_pipeline.layout, 0, 1, &state->global_descriptor_sets[index], 0, 0);
                }
                vkCmdDrawIndexed(command_buffer, state->level_model.index_count, 1, 0, 0, 0);
            }
            vkCmdEndRenderPass(command_buffer);

            VkRenderPassBeginInfo entity_render_pass_begin_info = {};
            entity_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            entity_render_pass_begin_info.renderPass = state->entity_pipeline.pass;
            entity_render_pass_begin_info.framebuffer = state->swapchain_framebuffers[index];
            entity_render_pass_begin_info.renderArea.offset.x = 0;
            entity_render_pass_begin_info.renderArea.offset.y = 0;
            entity_render_pass_begin_info.renderArea.extent.width = state->swapchain_extent.width;
            entity_render_pass_begin_info.renderArea.extent.height = state->swapchain_extent.height;
            entity_render_pass_begin_info.clearValueCount = 2;
            entity_render_pass_begin_info.pClearValues = clear_values;

            RenderEntityListInfo render_entity_list_info = {};
            render_entity_list_info.command_buffer = command_buffer;
            render_entity_list_info.begin_info = &entity_render_pass_begin_info;
            render_entity_list_info.subpass_contents = VK_SUBPASS_CONTENTS_INLINE;
            render_entity_list_info.pipeline = state->entity_pipeline;
            render_entity_list_info.descriptor_set = &state->global_descriptor_sets[index];

            render_entity_list(&render_entity_list_info, &state->doors);
            render_entity_list(&render_entity_list_info, &state->enemies);
            render_entity_list(&render_entity_list_info, &state->keycards);

            VkRenderPassBeginInfo crosshair_render_pass_begin_info = {};
            crosshair_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            crosshair_render_pass_begin_info.renderPass = state->crosshair_pipeline.pass;
            crosshair_render_pass_begin_info.framebuffer = state->swapchain_framebuffers[index];
            crosshair_render_pass_begin_info.renderArea.offset.x = 0;
            crosshair_render_pass_begin_info.renderArea.offset.y = 0;
            crosshair_render_pass_begin_info.renderArea.extent.width = state->swapchain_extent.width;
            crosshair_render_pass_begin_info.renderArea.extent.height = state->swapchain_extent.height;
            crosshair_render_pass_begin_info.clearValueCount = 2;
            crosshair_render_pass_begin_info.pClearValues = clear_values;

            vkCmdBeginRenderPass(command_buffer, &crosshair_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
            {
                VkBuffer vertex_buffers[] = {state->crosshair_color_buffer.buffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->crosshair_pipeline.pipeline);
                vkCmdDraw(command_buffer, 3, 1, 0, 0);
            }
            vkCmdEndRenderPass(command_buffer);
        }

        if(vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
            panic("Failed to end command buffer!");
        }
    }
}

void create_descriptor_pool(GameState* state) {
    VkDescriptorPoolSize ubo_pool_size = {};
    ubo_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_pool_size.descriptorCount = state->swapchain_image_count;

    VkDescriptorPoolSize sampler_pool_size = {};
    sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_pool_size.descriptorCount = state->swapchain_image_count;

    VkDescriptorPoolSize light_pool_size = {};
    light_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    light_pool_size.descriptorCount = state->swapchain_image_count;

    VkDescriptorPoolSize pool_sizes[descriptor_count] = {ubo_pool_size, sampler_pool_size, light_pool_size};

    VkDescriptorPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.poolSizeCount = descriptor_count;
    pool_create_info.pPoolSizes = pool_sizes;
    pool_create_info.maxSets = state->swapchain_image_count;

    if(vkCreateDescriptorPool(state->device, &pool_create_info, 0, &state->global_descriptor_pool) != VK_SUCCESS) {
        panic("Failed to create uniform descriptor pool!");
    }
}

void create_descriptor_sets(GameState* state) {
    VkDescriptorSetLayout* layouts = sbmalloc(&state->level_buffer, state->swapchain_image_count * sizeof(VkDescriptorSetLayout));
    for(usize index = 0; index < state->swapchain_image_count; index += 1) { layouts[index] = state->global_descriptor_set_layout; }

    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = state->global_descriptor_pool;
    allocate_info.descriptorSetCount = state->swapchain_image_count;
    allocate_info.pSetLayouts = layouts;

    state->global_descriptor_sets = sbmalloc(&state->level_buffer, state->swapchain_image_count * sizeof(VkDescriptorSet));
    if (vkAllocateDescriptorSets(state->device, &allocate_info, state->global_descriptor_sets) != VK_SUCCESS) {
        panic("Failed to allocate uniform descriptor sets!");
    }

    for(usize index = 0; index < state->swapchain_image_count; index += 1) {
        VkDescriptorSet descriptor_set = state->global_descriptor_sets[index];

        VkDescriptorBufferInfo uniform_buffer_info = {};
        uniform_buffer_info.buffer = state->camera_uniforms[index].buffer;
        uniform_buffer_info.offset = 0;
        uniform_buffer_info.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = state->level_texture.view;
        image_info.sampler = state->generic_sampler;

        VkDescriptorBufferInfo lights_buffer_info = {};
        lights_buffer_info.buffer = state->light_buffer.buffer;
        lights_buffer_info.offset = 0;
        lights_buffer_info.range = light_count * sizeof(Light);

        VkWriteDescriptorSet descriptor_writes[descriptor_count] = {
            {   .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_set,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo = &uniform_buffer_info,
                .pImageInfo = 0,
                .pTexelBufferView = 0,
            },
            {   .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_set,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pBufferInfo = 0,
                .pImageInfo = &image_info,
                .pTexelBufferView = 0,
            },
            {   .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_set,
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo = &lights_buffer_info,
                .pImageInfo = 0,
                .pTexelBufferView = 0,
            }
        };

        vkUpdateDescriptorSets(state->device, descriptor_count, descriptor_writes, 0, 0);
    }
}

void init_loader(GameState* state, LoaderState** loader) {
    *loader = malloc(sizeof(LoaderState));

    sbinit(&(*loader)->scratch, 1 * 1024 * 1024); // 1M
    sbinit(&(*loader)->read_scratch, 2 * 1024 * 1024); // 2M
}

void free_loader(GameState* state, LoaderState* loader) {
    stbi_image_free(loader->texture_pixels);
    sbfree(&loader->read_scratch);
    sbfree(&loader->scratch);
    free(loader);
}

void create_level_buffers(GameState* state, LoaderState* loader) {
    create_model(
        state->device,
        state->physical_device,
        state->queue,
        state->command_pool,
        sizeof(Vertex),
        loader->level_vertex_count,
        loader->level_vertices,
        loader->level_index_count,
        loader->level_indices,
        &state->level_model
    );
}

void create_entity_render_data(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkQueue queue,
    VkCommandPool command_pool,
    u32 vertex_count,
    void* vertices,
    u32 index_count,
    void* indices,
    EntityList* entities
) {
    create_model(
        device,
        physical_device,
        queue,
        command_pool,
        sizeof(Vertex),
        vertex_count,
        vertices,
        index_count,
        indices,
        &entities->model
    );

    create_device_local_and_staging_buffer(
        device,
        physical_device,
        queue,
        command_pool,
        sizeof(vec4) * entities->capacity,
        entities->position_rotations,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        &entities->position_rotation_buffer,
        &entities->position_rotation_staging_buffer
    );

    create_device_local_and_staging_buffer(
        device,
        physical_device,
        queue,
        command_pool,
        sizeof(vec4) * entities->capacity,
        entities->colors,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        &entities->color_buffer,
        &entities->color_staging_buffer
    );
}

void create_all_entity_buffers(GameState* state, LoaderState* loader) {
    create_entity_render_data(
        state->device,
        state->physical_device,
        state->queue,
        state->command_pool,
        enemy_vertex_count,
        enemy_vertices,
        enemy_index_count,
        enemy_indices,
        &state->enemies
    );

    create_entity_render_data(
        state->device,
        state->physical_device,
        state->queue,
        state->command_pool,
        loader->door_vertex_count,
        loader->door_vertices,
        loader->door_index_count,
        loader->door_indices,
        &state->doors
    );

    create_entity_render_data(
        state->device,
        state->physical_device,
        state->queue,
        state->command_pool,
        enemy_vertex_count,
        enemy_vertices,
        enemy_index_count,
        enemy_indices,
        &state->keycards
    );
}

void create_level_texture(GameState* state, LoaderState* loader) {
    create_device_local_image(
        state->device,
        state->physical_device,
        state->queue,
        state->command_pool,
        loader->texture_width,
        loader->texture_height,
        loader->image_size,
        loader->texture_pixels,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        &state->level_texture.image,
        &state->level_texture.memory
    );

    create_image_view(
        state->device,
        state->level_texture.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &state->level_texture.view
    );
}

void reset_player(GameState* state) {
    state->theta = 1.15;
    state->phi = 0.75;

    state->player_radius = 1.0f;
    state->player_z_speed = 0.0f;

    state->player_position.x = 0.0;
    state->player_position.y = 0.0;
    state->player_position.z = 0.0;

    state->player_keycards = KEYCARD_NONE;
}

void load_level(GameState* state) {
    LoaderState* loader;

    init_loader(state, &loader);
    {
        load_level_texture(state, loader);
        create_level_texture(state, loader);

        load_level_model(state, loader);
        create_level_buffers(state, loader);
        create_all_entity_buffers(state, loader);
        //create_level_buffers(state, loader);
        //create_door_buffers(state, loader);

        load_level_sounds(state, loader);
    }
    free_loader(state, loader);

    create_descriptor_pool(state);
    create_descriptor_sets(state);

    create_command_buffers(state);
}

void destroy_entity_list(VkDevice device, EntityList* entity_list) {
    destroy_model(device, entity_list->model);
    destroy_local_and_staging_buffer(device, entity_list->position_rotation_buffer, entity_list->position_rotation_staging_buffer);
    destroy_local_and_staging_buffer(device, entity_list->color_buffer, entity_list->color_staging_buffer);
}

void unload_level(GameState* state) {
    state->load_next_level = false;
    state->level_index += 1;

    vkDeviceWaitIdle(state->device);

    sbclear(&state->level_buffer);
    //sbclear(&state->level_buffer);
    //sbclear(&state->level_buffer);

    destroy_texture(state->device, state->level_texture);
    destroy_model(state->device, state->level_model);

    destroy_entity_list(state->device, &state->enemies);
    destroy_entity_list(state->device, &state->doors);
    destroy_entity_list(state->device, &state->keycards);

    vkDestroyDescriptorPool(state->device, state->global_descriptor_pool, 0);
    vkFreeCommandBuffers(state->device, state->command_pool, (u32)state->swapchain_image_count, state->command_buffers);

    reset_player(state);
}

#define THREE_HITS_LOAD_H

#endif //THREE_HITS_LOAD_H
