//
// Created by Windows Vista on 8/9/2021.
//

#ifndef UNTITLED_FPS_PHYSICS_H
#include "vmmath.h"

/*
typedef struct Sphere {
    vec3 position;
    f32 radius;
} Sphere;
*/

vec3 closest_point_on_line_segment(vec3 A, vec3 B, vec3 point) {
    vec3 AB = vec3_sub_vec3(B, A);
    f32 t = vec3_dot(vec3_sub_vec3(point, A), AB) / vec3_dot(AB, AB);
    //return vec3_mul_vec3(vec3_add_f32(A, f32_saturate(t)), AB);
    return vec3_add_vec3(A, vec3_mul_f32(AB, fminf(fmaxf(t, 0.0f), 1.0f)));
}

//TODO(sean): get this to work with vertex - index data
//TODO(sean): move this to just use re-duped vertex data
//TODO(sean): line intersection
b32 sphere_collides_with_triangle(vec3 A, vec3 B, vec3 C, vec3 P, f32 r, vec3* N, f32* d) {
    u32 m = 0;
    vec3 n = {{0.0f, 0.0f, 0.0f}};
    if(tA == true && tA >= tB && tA >= tC) {
        flag += 1;
        vec3 temp = {{P.x - A.x, P.y - A.y, P.z - A.z}};
        n = vec3_add_vec3(n, temp);
    } else if (tB == true && tB >= tA && tB >= tC) {
        flag += 1;
        vec3 temp = {{P.x - B.x, P.y - B.y, P.z - B.z}};
        n = vec3_add_vec3(n, temp);
    } else if (tC == true && tC >= tA && tC >= tB) {
        flag += 1;
        vec3 temp = {{P.x - C.x, P.y - C.y, P.z - C.z}};
        n = vec3_add_vec3(n, temp);
    }

    if(flag > 0) {
        *N = vec3_norm(n);
        return true;
    }

    *N = vec3_norm(vec3_cross(vec3_sub_vec3(B, A), vec3_sub_vec3(C, A)));

    vec3 edge1, edge2, h, s, q;
    f32 a, f, u, v;
    edge1 = vec3_sub_vec3(B, A);
    edge2 = vec3_sub_vec3(C, A);
    h = vec3_cross(*N, edge2);
    a = vec3_dot(edge1, h);
    f = 1.0/a;
    s = vec3_sub_vec3(P, A);
    u = f * vec3_dot(s, h);
    if(u < 0.0 || u > 1.0) { return false; }
    q = vec3_cross(s, edge1);
    v = f * vec3_dot(*N, q);
    if(v < 0.0 || (u + v) > 1.0) { return false; }

    f32 t = f * vec3_dot(edge2, q);
    if((t*t) < (r*r)) {
        return true;
    } else {
        return false;
    }
}

#define UNTITLED_FPS_PHYSICS_H

#endif //UNTITLED_FPS_PHYSICS_H
