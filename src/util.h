//
// Created by Sean Moulton on 7/22/2021.
//

#ifndef UNTITLED_FPS_UTILS_H

#include <stdint.h>

#define global static
#define local static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t usize;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef intptr_t isize;

#define false 0
#define true 1
typedef u8 b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

typedef float f32;
typedef double f64;

typedef struct DebugCallbackData {} DebugCallbackData;

typedef struct StagedBuffer {
    void* pointer;
    usize capacity;
    usize length;
} StagedBuffer;

global inline void* sbmalloc(StagedBuffer* heap, usize required_size) {
    usize would_be_offset = heap->length + required_size;

    if (would_be_offset > heap->capacity) {
        return 0;
    } else {
        void* pointer = (void*)(heap->pointer + heap->length);
        heap->length += required_size;
        return pointer;
    }
}

global inline void sbclear(StagedBuffer* heap) {
    heap->length = 0;
}

global inline void sbfree(StagedBuffer* heap) {
    free(heap->pointer);
    heap->capacity = 0;
    heap->length = 0;
}

global inline void* sbcalloc(StagedBuffer* heap, u8 clear_value, usize required_size) {
    usize would_be_offset = heap->length + required_size;

    if (would_be_offset > heap->capacity) {
        return 0;
    } else {
        // set required_size bytes to 0 at position pointer + length
        void* pointer = heap->pointer + heap->length;
        memset(pointer, clear_value, required_size);

        heap->length += required_size;
        return pointer;
    }
}

global inline void sbinit(StagedBuffer* heap, usize allocation_size) {
    heap->length = 0;
    heap->capacity = allocation_size;
    heap->pointer = malloc(allocation_size);
}

#define panic(message) \
    fprintf(stderr, "Thread panicked at message: \"%s\"\n", message); \
    exit(1);

#define UNTITLED_FPS_UTILS_H
#endif //UNTITLED_FPS_UTILS_H
