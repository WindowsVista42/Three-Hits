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
    generate_sound_sources(&state->audio_buffer, &state->enemy_alert_sound_sources, state->max_enemy_count, 0.8f, AL_FALSE, state->enemy_alert_sound_buffer, state->reverb_slot);

    generate_sound_sources(&state->audio_buffer, &state->enemy_gun_sound_sources, state->max_enemy_count, 1.0f, AL_FALSE, state->enemy_gun_sound_buffer, state->reverb_slot);
    set_sources_f(state->enemy_gun_sound_sources, state->max_enemy_count, AL_ROLLOFF_FACTOR, 0.2);

    generate_sound_sources(&state->audio_buffer, &state->enemy_windup_sound_sources, state->max_enemy_count, 0.5f, AL_FALSE, state->enemy_windup_sound_buffer, state->reverb_slot);
    set_sources_f(state->enemy_windup_sound_sources, state->max_enemy_count, AL_ROLLOFF_FACTOR, 0.8);

    generate_sound_sources(&state->audio_buffer, &state->door_sound_sources, state->door_count, 1.2f, AL_FALSE, state->door_opening_sound_buffer, state->reverb_slot);

    generate_sound_sources(&state->audio_buffer, &state->enemy_ambience_sound_sources, state->max_enemy_count, 0.8f, AL_TRUE, state->enemy_ambience_sound_buffer, state->reverb_slot);
    set_sources_f(state->enemy_ambience_sound_sources, state->max_enemy_count, AL_ROLLOFF_FACTOR, 3.0);
    play_sources(state->enemy_ambience_sound_sources, state->enemy_alive_count);
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
        fread(&(*vertices)[index].pos.x, sizeof(f32), 1, fp);
        fread(&(*vertices)[index].pos.y, sizeof(f32), 1, fp);
        fread(&(*vertices)[index].pos.z, sizeof(f32), 1, fp);

        fread(&(*vertices)[index].color.x, sizeof(f32), 1, fp);
        fread(&(*vertices)[index].color.y, sizeof(f32), 1, fp);
        fread(&(*vertices)[index].color.z, sizeof(f32), 1, fp);

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

void load_level_model(GameState* state, LoaderState* loader) {
    loader->level_path = "../data/levels/doors0.level";

    {
        FILE *fp = fopen(loader->level_path, "rb");
        panic_if_zero(fp, "Failed to open level!");

        // load level model
        read_vertices(&loader->read_scratch, &loader->level_vertex_count, &loader->level_vertices, fp);
        read_indices(&loader->read_scratch, &loader->level_index_count, &loader->level_indices, fp);
        state->level_model.index_count = loader->level_index_count;

        read_vertices(&loader->read_scratch, &loader->door_vertex_count, &loader->door_vertices, fp);
        read_indices(&loader->read_scratch, &loader->door_index_count, &loader->door_indices, fp);
        state->door_model.index_count = loader->door_index_count;

        // load physmesh
        fread(&state->physmesh_vertex_count, sizeof(u32), 1, fp);
        state->physmesh_vertices = sbmalloc(&state->physics_buffer, state->physmesh_vertex_count * sizeof(vec3));
        for (usize index = 0; index < state->physmesh_vertex_count; index += 1) {
            fread(&state->physmesh_vertices[index].x, sizeof(f32), 1, fp);
            fread(&state->physmesh_vertices[index].y, sizeof(f32), 1, fp);
            fread(&state->physmesh_vertices[index].z, sizeof(f32), 1, fp);
        }

        // load door prs
        fread(&state->door_count, sizeof(u32), 1, fp);
        state->door_position_rotations = sbmalloc(&state->physics_buffer, state->door_count * sizeof(vec4));
        for (usize index = 0; index < state->door_count; index += 1) {
            fread(&state->door_position_rotations[index].x, sizeof(f32), 1, fp);
            fread(&state->door_position_rotations[index].y, sizeof(f32), 1, fp);
            fread(&state->door_position_rotations[index].z, sizeof(f32), 1, fp);
            fread(&state->door_position_rotations[index].w, sizeof(f32), 1, fp);
        }

        // load doors physmesh ranges
        fread(&state->door_physmesh_range_count, sizeof(u32), 1, fp);
        state->door_physmesh_ranges = sbmalloc(&state->physics_buffer, state->door_physmesh_range_count * sizeof(u32));
        for (usize index = 0; index < state->door_physmesh_range_count; index += 1) {
            fread(&state->door_physmesh_ranges[index], sizeof(u32), 1, fp);
        }

        fclose(fp);
    }

    state->level_model.index_count = loader->level_index_count;
    state->door_timings = sbcalloc(&state->physics_buffer, 0, state->door_count * sizeof(f32));
    state->door_colors = sbmalloc(&state->physics_buffer, state->door_count * sizeof(vec4));
    for(usize index = 0; index < state->door_count; index += 1) {
        state->door_colors[index] = vec4_new(1.0, 1.0, 0.0, 1.0);
    }

    // load enemies
    {
        state->max_enemy_count = 30;
        state->enemy_alive_count = 0;//state->max_enemy_count;
        u32 i = 0;

        state->enemy_position_rotations = sbmalloc(&state->physics_buffer, state->max_enemy_count * sizeof(vec4));
        state->enemy_hit_times = sbmalloc(&state->enemy_buffer, state->max_enemy_count * sizeof(f32));
        state->enemy_colors = sbmalloc(&state->enemy_buffer, state->max_enemy_count * sizeof(vec4));
        state->enemy_healths = sbmalloc(&state->enemy_buffer, state->max_enemy_count * sizeof(i32));
        state->enemy_shoot_times = sbcalloc(&state->enemy_buffer, 0, state->max_enemy_count * sizeof(f32));

        // starting room
        state->enemy_position_rotations[i++] = vec4_new(-4.5, -20.0, 1.0, 0.0);

        // right hallway
        state->enemy_position_rotations[i++] = vec4_new(-23.5, -31.0, 1.0, 0.0);

        // right room
        state->enemy_position_rotations[i++] = vec4_new(-26.0, -16.0, 1.0, 0.0);
        state->enemy_position_rotations[i++] = vec4_new(-24.0, -8.0 , 1.0, 0.0);
        state->enemy_position_rotations[i++] = vec4_new(-32.5,  0.25, 1.0, 0.0);
        state->enemy_position_rotations[i++] = vec4_new(-37.0,  0.25, 1.0, 0.0);
        state->enemy_position_rotations[i++] = vec4_new(-43.0, -9.5 , 1.0, 0.0);

        // upper hallway
        state->enemy_position_rotations[i++] = vec4_new(-5.25, -58.5, 10.5, 0.0);
        state->enemy_position_rotations[i++] = vec4_new( 5.5,  -58.5, 10.5, 0.0);
        state->enemy_position_rotations[i++] = vec4_new(18.0,  -64.0, 10.5, 0.0);

        // upper left room
        state->enemy_position_rotations[i++] = vec4_new(20.5,  -73.0, 10.5, 0.0);
        state->enemy_position_rotations[i++] = vec4_new(15.5,  -72.5, 10.5, 0.0);
        state->enemy_position_rotations[i++] = vec4_new(29.0,  -72.0, 10.5, 0.0);

        state->enemy_position_rotations[i++] = vec4_new(25.25, -88.0, 10.5, 0.0);
        state->enemy_position_rotations[i++] = vec4_new(26.0,  -83.0, 10.5, 0.0);
        state->enemy_position_rotations[i++] = vec4_new( 9.0,  -74.5, 10.5, 0.0);
        state->enemy_position_rotations[i++] = vec4_new( 9.0,  -78.5, 10.5, 0.0);

        // upper right room
        state->enemy_position_rotations[i++] = vec4_new(-17.5, -64.5, 10.5, 0.0);
        state->enemy_position_rotations[i++] = vec4_new(-16.5, -75.0, 10.5, 0.0);
        state->enemy_position_rotations[i++] = vec4_new(-25.5, -65.5, 10.5, 0.0);
        state->enemy_position_rotations[i++] = vec4_new(-30.0, -71.5, 10.5, 0.0);

        // bottom of stairs
        state->enemy_position_rotations[i++] = vec4_new( 9.5,  -54.5, -10.5, 0.0);

        // rest
        state->enemy_alive_count = i;
        for(u32 index = state->enemy_alive_count; index < state->max_enemy_count; index += 1) {
            state->enemy_position_rotations[index].x = 100.0 + (f32)i;
            state->enemy_position_rotations[index].y = 100.0 + (f32)i;
            state->enemy_position_rotations[index].z = 1.0f;
            state->enemy_position_rotations[index].w = 0.0f;
        }

        // other init data
        for(u32 index = 0; index < state->enemy_alive_count; index += 1) {
            state->enemy_colors[index].x = 1.0;
            state->enemy_colors[index].y = 0.0;
            state->enemy_colors[index].z = 0.0;
        }

        const i32 enemy_default_health = 4;
        for(u32 index = 0; index < state->enemy_alive_count; index += 1) {
            state->enemy_healths[index] = enemy_default_health;
        }

        for(u32 index = 0; index < state->enemy_alive_count; index += 1) {
            state->enemy_hit_times[index] = 0.0;
        }
    }

    {
        state->enemy_sees_player = sbmalloc(&state->audio_buffer, state->max_enemy_count * sizeof(b32));
        state->windup_needs_reverse = sbmalloc(&state->audio_buffer, state->max_enemy_count * sizeof(b32));
    }

    // load healthpacks
    {
        /*
        state->max_healthpack_count = 10;
        state->healthpack_count = state->max_healthpack_count;
        u32 i = 0;

        state->healthpack_position_rotations = sbmalloc(&state->physics_buffer, state->max_enemy_count * sizeof(vec4));
        state->healthpack_colors = sbmalloc(&state->enemy_buffer, state->max_enemy_count * sizeof(vec4));

        // rest
        state->healthpack_count = i;
        for(u32 index = state->healthpack_count; index < state->max_healthpack_count; index += 1) {
            state->healthpack_position_rotations[index].x = 100.0 + (f32)i;
            state->healthpack_position_rotations[index].y = 100.0 + (f32)i;
            state->healthpack_position_rotations[index].z = 1.0f;
            state->healthpack_position_rotations[index].w = 0.0f;
        }
        */
    }

    // load ammopacks
    {

    }

    // load keycards
    {

    }

    // load shadow-casting lights
    {
    }
    // X Y Z R
    // R G B I
    // load fill lights

}

#define THREE_HITS_LOAD_H

#endif //THREE_HITS_LOAD_H
