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

global inline vec4 vec4_splat(f32 splat) {
    vec4 vector = {{splat, splat, splat, splat}};
    return vector;
}

global inline vec3 vec3_sub_vec3(vec3, vec3);
global inline f32 vec3_length(vec3 vector);
global inline f32 vec3_length_recip(vec3 vector);
global inline vec3 vec3_norm(vec3 vector);
global inline vec3 vec3_cross(vec3 lhs, vec3 rhs);
global inline f32 vec3_dot(vec3 lhs, vec3 rhs);

global inline mat4 mat4_splat(f32 splat) {
    mat4 matrix = {{
        splat, splat, splat, splat,
        splat, splat, splat, splat,
        splat, splat, splat, splat,
        splat, splat, splat, splat,
    }};
    return matrix;
}

global inline vec3 vec3_mul_scalar(vec3 vector, f32 scalar) {
    vec3 output;
    output.x = vector.x * scalar;
    output.y = vector.y * scalar;
    output.z = vector.z * scalar;
    return output;
}

global inline vec3 vec3_mul_vec3(vec3 lhs, vec3 rhs) {
    vec3 output;
    output.x = lhs.x * rhs.x;
    output.y = lhs.y * rhs.y;
    output.z = lhs.z * rhs.z;
    return output;
}

global inline vec4 vec4_unit_w() {
    vec4 output = {{0.0f, 0.0f, 0.0f, 1.0f}};
    return output;
}

global inline mat4 mat4_rotate(mat4 matrix, f32 angle, vec3 axis) {
    f32 sin = sinf(angle);
    f32 cos = cosf(angle);
    vec3 axis_sin = vec3_mul_scalar(axis, sin);
    vec3 axis_sq = vec3_mul_vec3(axis, axis);
    f32 omc = 1.0 - cos;
    f32 xyomc = axis.x * axis.y * omc;
    f32 xzomc = axis.x * axis.z * omc;
    f32 yzomc = axis.y * axis.z * omc;

    mat4 output = {
        .xs = {{
            axis_sq.x * omc + cos,
            xyomc + axis_sin.z,
            xzomc - axis_sin.y,
            0.0f,
        }},
        .ys = {{
            xyomc - axis_sin.z,
            axis_sq.y * omc + cos,
            yzomc + axis_sin.x,
            0.0f,
        }},
        .zs = {{
            xzomc + axis_sin.y,
            yzomc - axis_sin.x,
            axis_sq.z * omc + cos,
            0.0f,
        }},
        .ws = vec4_unit_w(),
    };

    return output;
}

global inline vec3 vec3_sub_vec3(vec3 lhs, vec3 rhs) {
    vec3 output = {{
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z
    }};
    return output;
}

global inline f32 vec3_length(vec3 vector) {
    return sqrtf(vec3_dot(vector, vector));
}

global inline f32 vec3_length_recip(vec3 vector) {
    return 1.0 / vec3_length(vector);
}

global inline vec3 vec3_norm(vec3 vector) {
    return vec3_mul_scalar(vector, vec3_length_recip(vector));
}

global inline vec3 vec3_cross(vec3 lhs, vec3 rhs) {
    vec3 output = {{
        lhs.y * rhs.z - rhs.y * lhs.z,
        lhs.z * rhs.x - rhs.z * lhs.x,
        lhs.x * rhs.y - rhs.x * lhs.y
    }};
    return output;
}

global inline f32 vec3_dot(vec3 lhs, vec3 rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

global inline mat4 mat4_look_at(vec3 eye, vec3 center, vec3 up) {
    vec3 dir = vec3_sub_vec3(eye, center);
    vec3 f = vec3_norm(dir);
    vec3 s = vec3_norm(vec3_cross(up, f));
    vec3 u = vec3_cross(f, s);

    mat4 output = {
        .xs = {{s.x, u.x, f.x, 0.0f}},
        .ys = {{s.y, u.y, f.y, 0.0f}},
        .zs = {{s.z, u.z, f.z, 0.0f}},
        .ws = {{-vec3_dot(s, eye), -vec3_dot(u, eye), -vec3_dot(f, eye), 1.0f}},
    };

    return output;
}

global inline mat4 mat4_perspective(f32 fov, f32 asp, f32 z_near, f32 z_far) {
    f32 inv_length = 1.0f / (z_near - z_far);
    f32 f = 1.0f / tanf((0.5f * fov));
    f32 a = f / asp;
    f32 b = (z_near + z_far) * inv_length;
    f32 c = (2.0f * z_near * z_far) * inv_length;

    mat4 output = {
        .xs = {{a,    0.0f, 0.0f,  0.0f}},
        .ys = {{0.0f, f,    0.0f,  0.0f}},
        .zs = {{0.0f, 0.0f, b,    -1.0f}},
        .ws = {{0.0f, 0.0f, c,     0.0f}},
    };

    return output;
}

#define UNTITLED_FPS_VMMATH_H

#endif //UNTITLED_FPS_VMMATH_H
