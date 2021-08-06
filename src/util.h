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

global inline void panic(char* message) {
    fprintf(stderr, "Thread panicked at message: \"%s\"\n", message);
    //fprintf(stderr, "Press enter to continue...");
    //getchar();
    exit(1);
}

// platform dependant
typedef struct Timer Timer;
Timer timer_new();
void timer_init(Timer* timer);
void timer_start(Timer* timer);
void timer_end(Timer* timer);

#ifdef WIN64
#include <windows.h>

typedef struct Timer {
    LARGE_INTEGER frequency, t1, t2;
    f64 elapsed;
} Timer;

Timer timer_new() {
    Timer timer;
    timer_init(&timer);
    return timer;
}

void timer_init(Timer* timer) {
    QueryPerformanceFrequency(&timer->frequency);
}

void timer_start(Timer* timer) {
    QueryPerformanceCounter(&timer->t1);
}

void timer_end(Timer* timer) {
    QueryPerformanceCounter(&timer->t2);
    timer->elapsed = (f64)(timer->t2.QuadPart - timer->t1.QuadPart) / timer->frequency.QuadPart;
}
#endif

#ifdef __unix__
#endif

#define UNTITLED_FPS_UTILS_H
#endif //UNTITLED_FPS_UTILS_H

/*
typedef struct FlatVector {
    void* data;
    usize data_length;

    void* offsets;
    usize offsets_length;
} FlatVector;
*/

