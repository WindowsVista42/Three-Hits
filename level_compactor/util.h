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

#endif