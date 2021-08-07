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

global const vec3 VEC3_ZERO = {{0.0f, 0.0f, 0.0f}};
global const vec4 VEC4_UNIT_W = {{0.0f, 0.0f, 0.0f, 1.0f}};

global inline vec4 vec4_f32(f32 splat) {
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

global inline vec3 vec3_mul_f32(vec3 vector, f32 scalar) {
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

global inline mat4 mat4_rotate(mat4 matrix, f32 angle, vec3 axis) {
    f32 sin = sinf(angle);
    f32 cos = cosf(angle);
    vec3 axis_sin = vec3_mul_f32(axis, sin);
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
        .ws = VEC4_UNIT_W,
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

global inline mat4 mat4_mul_mat4(mat4 a, mat4 b) {
    mat4 output = {
        .xs = {{
            a.xs.x*b.xs.x + a.xs.y*b.ys.x + a.xs.z*b.zs.x + a.xs.w*b.ws.x,
            a.xs.x*b.xs.y + a.xs.y*b.ys.y + a.xs.z*b.zs.y + a.xs.w*b.ws.y,
            a.xs.x*b.xs.z + a.xs.y*b.ys.z + a.xs.z*b.zs.z + a.xs.w*b.ws.z,
            a.xs.x*b.xs.w + a.xs.y*b.ys.w + a.xs.z*b.zs.w + a.xs.w*b.ws.w,
        }},
        .ys = {{
            a.ys.x*b.xs.x + a.ys.y*b.ys.x + a.ys.z*b.zs.x + a.ys.w*b.ws.x,
            a.ys.x*b.xs.y + a.ys.y*b.ys.y + a.ys.z*b.zs.y + a.ys.w*b.ws.y,
            a.ys.x*b.xs.z + a.ys.y*b.ys.z + a.ys.z*b.zs.z + a.ys.w*b.ws.z,
            a.ys.x*b.xs.w + a.ys.y*b.ys.w + a.ys.z*b.zs.w + a.ys.w*b.ws.w,
        }},
        .zs = {{
            a.zs.x*b.xs.x + a.zs.y*b.ys.x + a.zs.z*b.zs.x + a.zs.w*b.ws.x,
            a.zs.x*b.xs.y + a.zs.y*b.ys.y + a.zs.z*b.zs.y + a.zs.w*b.ws.y,
            a.zs.x*b.xs.z + a.zs.y*b.ys.z + a.zs.z*b.zs.z + a.zs.w*b.ws.z,
            a.zs.x*b.xs.w + a.zs.y*b.ys.w + a.zs.z*b.zs.w + a.zs.w*b.ws.w,
        }},
        .ws = {{
            a.ws.x*b.xs.x + a.ws.y*b.ys.x + a.ws.z*b.zs.x + a.ws.w*b.ws.x,
            a.ws.x*b.xs.y + a.ws.y*b.ys.y + a.ws.z*b.zs.y + a.ws.w*b.ws.y,
            a.ws.x*b.xs.z + a.ws.y*b.ys.z + a.ws.z*b.zs.z + a.ws.w*b.ws.z,
            a.ws.x*b.xs.w + a.ws.y*b.ys.w + a.ws.z*b.zs.w + a.ws.w*b.ws.w,
        }},
    };
    return output;
}

global inline f32 f32_wrap(f32 x, f32 wrap) {
    if(x > wrap) {
        x -= wrap;
    }
    return x;
}

global inline f32 vec3_length_recip(vec3 vector) {
    return 1.0 / vec3_length(vector);
}

global inline vec3 vec3_norm(vec3 vector) {
    return vec3_mul_f32(vector, vec3_length_recip(vector));
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

global inline f32 f32_clamp(f32 x, f32 min, f32 max) {
    if(x < min) {
        return min;
    }
    if(x > max) {
        return max;
    }
    return x;
}

global inline f32 f32_radians(f32 degrees) {
    return (M_PI/180.0f) * degrees;
}

/*
global inline vec3 vec3_zero() {
    vec3 output = {{0.0, 0.0, 0.0}};
    return output;
}
*/

global inline f32 f32_abs(f32 x) {
    if(x < 0.0) {
        return -x;
    } else {
        return x;
    }
}

// True if all fields are equal
global inline b32 vec3_eq_vec3(vec3 lhs, vec3 rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

// True if one or more fields are equal
global inline b32 vec3_par_eq_vec3(vec3 lhs, vec3 rhs) {
    return lhs.x == rhs.x || lhs.y == rhs.y || lhs.z == rhs.z;
}

// True if one or more fields are not equal
global inline b32 vec3_ne_vec3(vec3 lhs, vec3 rhs) {
    return lhs.x != rhs.x && lhs.y != rhs.y && lhs.z != rhs.z;
}

// True if one or more fields are not equal
global inline b32 vec3_par_ne_vec3(vec3 lhs, vec3 rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

global inline vec3 vec3_from_theta_phi(f32 theta, f32 phi) {
    vec3 output = {{
        cosf(phi) * sinf(theta),
        sinf(phi) * sinf(theta),
        cosf(theta),
    }};
    return output;
}

global inline vec3 vec3_add_vec3(vec3 lhs, vec3 rhs) {
    vec3 output = {{
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
    }};
    return output;
}

global mat4 mat4_look_dir(vec3 eye, vec3 dir, vec3 up) {
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
        .xs = {{a,     0.0f, 0.0f,  0.0f}},
        .ys = {{0.0f, -f,    0.0f,  0.0f}},
        .zs = {{0.0f,  0.0f, b,    -1.0f}},
        .ws = {{0.0f,  0.0f, c,     0.0f}},
    };

    return output;
}

#define UNTITLED_FPS_VMMATH_H

#endif //UNTITLED_FPS_VMMATH_H
