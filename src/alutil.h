//
// Created by Windows Vista on 8/17/2021.
//

#ifndef UNTITLED_FPS_ALUTIL_H

#include <stdio.h>

#include "util.h"

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"
#include "AL/efx.h"
#include "AL/efx-presets.h"

/* Effect object functions */
global LPALGENEFFECTS alGenEffects;
global LPALDELETEEFFECTS alDeleteEffects;
global LPALISEFFECT alIsEffect;
global LPALEFFECTI alEffecti;
global LPALEFFECTIV alEffectiv;
global LPALEFFECTF alEffectf;
global LPALEFFECTFV alEffectfv;
global LPALGETEFFECTI alGetEffecti;
global LPALGETEFFECTIV alGetEffectiv;
global LPALGETEFFECTF alGetEffectf;
global LPALGETEFFECTFV alGetEffectfv;

/* Auxiliary Effect Slot object functions */
global LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
global LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
global LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
global LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
global LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
global LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
global LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
global LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
global LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
global LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
global LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;

typedef ALuint SoundEffect;
typedef ALuint SoundSlot;
typedef ALuint SoundBuffer;
typedef ALuint SoundSource;
typedef EFXEAXREVERBPROPERTIES ReverbProperties;
typedef ALCcontext AlContext;
typedef ALCdevice AlDevice;

void load_al_procedures() {
#define LOAD_PROC(T, x)  ((x) = (T)alGetProcAddress(#x))
    LOAD_PROC(LPALGENEFFECTS, alGenEffects);
    LOAD_PROC(LPALDELETEEFFECTS, alDeleteEffects);
    LOAD_PROC(LPALISEFFECT, alIsEffect);
    LOAD_PROC(LPALEFFECTI, alEffecti);
    LOAD_PROC(LPALEFFECTIV, alEffectiv);
    LOAD_PROC(LPALEFFECTF, alEffectf);
    LOAD_PROC(LPALEFFECTFV, alEffectfv);
    LOAD_PROC(LPALGETEFFECTI, alGetEffecti);
    LOAD_PROC(LPALGETEFFECTIV, alGetEffectiv);
    LOAD_PROC(LPALGETEFFECTF, alGetEffectf);
    LOAD_PROC(LPALGETEFFECTFV, alGetEffectfv);

    LOAD_PROC(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots);
    LOAD_PROC(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots);
    LOAD_PROC(LPALISAUXILIARYEFFECTSLOT, alIsAuxiliaryEffectSlot);
    LOAD_PROC(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti);
    LOAD_PROC(LPALAUXILIARYEFFECTSLOTIV, alAuxiliaryEffectSlotiv);
    LOAD_PROC(LPALAUXILIARYEFFECTSLOTF, alAuxiliaryEffectSlotf);
    LOAD_PROC(LPALAUXILIARYEFFECTSLOTFV, alAuxiliaryEffectSlotfv);
    LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTI, alGetAuxiliaryEffectSloti);
    LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTIV, alGetAuxiliaryEffectSlotiv);
    LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTF, alGetAuxiliaryEffectSlotf);
    LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTFV, alGetAuxiliaryEffectSlotfv);
#undef LOAD_PROC
}

typedef struct RawSound {
    u8 channels;
    i32 sample_rate;
    u8 bits_per_sample;
    ALsizei size;
    u8* data;
    ALenum format;
} RawSound;

//TODO(sean): use staged buffers
void load_wav(
    StagedBuffer* staged_buffer,
    char* file_name,
    RawSound* sound
) {
    char buffer[4];

    FILE* file = fopen(file_name, "rb");
    assert(file != 0);

    // "RIFF"
    fread(buffer, 4, 1, file);
    assert(strncmp(buffer, "RIFF", 4) == 0);

    // FILE SIZE
    fread(buffer, 4, 1, file);

    // "WAVE"
    fread(buffer, 4, 1, file);
    assert(strncmp(buffer, "WAVE", 4) == 0);

    // "fmt/0"
    fread(buffer, 4, 1, file);

    // SIZE OF FMT DATA CHUNK (always 16)
    fread(buffer, 4, 1, file);

    // PCM
    fread(buffer, 2, 1, file);

    // CHANNEL COUNT
    fread(buffer, 2, 1, file);
    sound->channels = *(uint8_t*)buffer;

    // SAMPLE RATE
    fread(buffer, 4, 1, file);
    sound->sample_rate = *(int32_t*)buffer;

    // (SAMPLE RATE * BITS PER CHANNEL * CHANNELS) / 8
    fread(buffer, 4, 1, file);

    // ?? fucking what is this
    fread(buffer, 2, 1, file);

    // BITS PER SAMPLE
    fread(buffer, 2, 1, file);
    sound->bits_per_sample = *(uint8_t*)buffer;

    // "data"
    fread(buffer, 4, 1, file);
    assert(strncmp(buffer, "data", 4) == 0);

    // SIZE OF DATA
    fread(buffer, 4, 1, file);
    sound->size = *(ALsizei*)buffer;

    sound->data = sbmalloc(staged_buffer, sound->size * sizeof(char));
    fread(sound->data, sound->size, 1, file);

    if(sound->channels == 1 && sound->bits_per_sample == 8) {
        sound->format = AL_FORMAT_MONO8;
    } else if (sound->channels == 1 && sound->bits_per_sample == 16) {
        sound->format = AL_FORMAT_MONO16;
    } else if (sound->channels == 2 && sound->bits_per_sample == 8) {
        sound->format = AL_FORMAT_STEREO8;
    } else if (sound->channels == 2 && sound->bits_per_sample == 16) {
        sound->format = AL_FORMAT_STEREO16;
    } else {
        panic("Failed to find sound format for sound!");
    }

    fclose(file);

    sbclear(staged_buffer);
}

void generate_sound_source(SoundSource* source, f32 gain, ALboolean looping, SoundBuffer buffer, SoundSlot slot) {
    alGenSources(1, source);
    alSourcef(*source, AL_GAIN, gain);
    alSourcei(*source, AL_LOOPING, looping);
    if(alIsBuffer(buffer) == AL_TRUE) { alSourcei(*source, AL_BUFFER, buffer); }
    if(alIsAuxiliaryEffectSlot(slot) == AL_TRUE) { alSource3i(*source, AL_AUXILIARY_SEND_FILTER, (ALint)slot, 0, AL_FILTER_NULL); }
}

void load_sound_buffer_and_data(SoundBuffer* buffer, RawSound* raw_sound){
    alGenBuffers(1, buffer);
    alBufferData(*buffer, raw_sound->format, raw_sound->data, raw_sound->size, raw_sound->sample_rate);
}

void load_sound(StagedBuffer* staged_buffer, char* file_path, SoundBuffer* buffer) {
    RawSound raw_sound;
    load_wav(staged_buffer, file_path, &raw_sound);
    load_sound_buffer_and_data(buffer, &raw_sound);
}

void set_sources_f(SoundSource* sources, usize count, ALenum param, ALfloat value) {
    for(usize index = 0; index < count; index += 1) {
        alSourcef(sources[index], param, value);
    }
}

void generate_sound_sources(StagedBuffer* staged_buffer, SoundSource** sources, usize count, f32 gain, ALboolean looping, SoundBuffer buffer, SoundSlot slot) {
    *sources = sbmalloc(staged_buffer, count * sizeof(SoundSource));
    alGenSources(count, *sources);
    for(usize index = 0; index < count; index += 1) {
        alSourcef((*sources)[index], AL_GAIN, gain);
        alSourcei((*sources)[index], AL_LOOPING, looping);
        if(alIsBuffer(buffer) == AL_TRUE) { alSourcei((*sources)[index], AL_BUFFER, buffer); }
        if(alIsAuxiliaryEffectSlot(slot) == AL_TRUE) { alSource3i((*sources)[index], AL_AUXILIARY_SEND_FILTER, (ALint)slot, 0, AL_FILTER_NULL); }
    }
}

void play_sources(SoundSource* sources, usize count) {
    for(usize index = 0; index < count; index += 1) {
        alSourcePlay(sources[index]);
    }
}

#define UNTITLED_FPS_ALUTIL_H

#endif //UNTITLED_FPS_ALUTIL_H
