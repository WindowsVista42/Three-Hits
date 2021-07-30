//
// Created by Windows Vista on 7/25/2021.
//

#ifndef UNTITLED_FPS_VMMATH_H

#include "util.h"
#include "math.h"

// 8 bytes
typedef union vec2 {
    f32 raw[2];
    struct { f32 x; f32 y; };
} vec2;

// 16 bytes
typedef union vec3 {
    f32 raw[3];
    struct { f32 x; f32 y; f32 z; };
} vec3;

// 16 bytes
typedef union vec4 {
    f32 raw[4];
    struct { f32 x; f32 y; f32 z; f32 w; };
} vec4;

// 16 bytes
typedef union mat2 {
    f32 raw[4];
    struct { vec2 xs; vec2 ys; };
} mat2;

// 36 bytes
typedef union mat3 {
    f32 raw[9];
    struct { vec3 xs; vec3 ys; vec3 zs; };
} mat3;

// 64 bytes
typedef union mat4 {
    f32 raw[16];
    struct { vec4 xs; vec4 ys; vec4 zs; vec4 ws; };
} mat4;


global inline vec2 add_vec2(vec2 lhs, vec2 rhs) {
    vec2 ret = {{lhs.x + rhs.x, lhs.y + rhs.y}};
    return ret;
}

global inline vec2 sub_vec2(vec2 lhs, vec2 rhs) {
    vec2 ret = {{lhs.x - rhs.x, lhs.y - rhs.y}};
    return ret;
}

global inline vec2 mul_vec2(vec2 lhs, vec2 rhs) {
    vec2 ret = {{lhs.x * rhs.x, lhs.y * rhs.y}};
    return ret;
}

global inline vec2 div_vec2(vec2 lhs, vec2 rhs) {
    vec2 ret = {{lhs.x / rhs.x, lhs.y / rhs.y}};
    return ret;
}

global inline f32 dotprod_vec2(vec2 lhs, vec2 rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

global inline f32 mag_vec2(vec2 lhs) {
    return sqrtf((lhs.x * lhs.x) + (lhs.y * lhs.y));
}

/// Squared magnitude of lhs
global inline f32 sqmag_vec2(vec2 lhs) {
    return (lhs.x * lhs.x) + (lhs.y * lhs.y);
}


#define UNTITLED_FPS_VMMATH_H

#endif //UNTITLED_FPS_VMMATH_H
