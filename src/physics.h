//
// Created by Windows Vista on 8/9/2021.
//

#ifndef UNTITLED_FPS_PHYSICS_H
#include "vmmath.h"

b32 ray_intersects_triangle(vec3 A, vec3 B, vec3 C, vec3 P, f32 r, vec3* N) {
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
    if((t*t) < (r*r)) { return true; }
    else { return false; }
}

b32 sphere_intersects_point(vec3 p, vec3 P, f32 r, vec3* N) {
    b32 tp;
    f32 dp = ((P.x - p.x) * (P.x - p.x)) + ((P.y - p.y) * (P.y - p.y)) + ((P.z - p.z) * (P.z - p.z));
    tp = (r * r) > dp;
    vec3 temp = {{P.x - p.x, P.y - p.y, P.z - p.z}};
    *N = vec3_norm(temp);
    return tp;
}

//TODO(sean): get this to work with vertex - index data
//TODO(sean): move this to just use re-duped vertex data
//TODO(sean): line intersection
b32 sphere_collides_with_triangle(vec3 A, vec3 B, vec3 C, vec3 P, f32 r, vec3* N, f32* d) {
    u32 m = 0;
    vec3 n = {{0.0f, 0.0f, 0.0f}};
    vec3 Rn, ABn, BCn, CAn, An, Bn, Cn;
    b32 Ri, ABi, BCi, CAi, Ai, Bi, Ci;

    Ri = ray_intersects_triangle(A, B, C, P, r, &Rn);
    //ABi = sphere_intersects_line(A, B, P, r, &ABn);
    //BCi = sphere_intersects_line(B, C, P, r, &BCn);
    //CAi = sphere_intersects_line(C, A, P, r, &CAn);
    Ai = sphere_intersects_point(A, P, r, &An);
    Bi = sphere_intersects_point(A, P, r, &Bn);
    Ci = sphere_intersects_point(A, P, r, &Cn);

    // intersects with top
    if(Ri) {
        n = vec3_add_vec3(n, Rn);
        *N = vec3_norm(n);
        return true;
    }

    // intersects with side

    // intersects with corner
    if(Ai) {
        m += 1;
        n = vec3_add_vec3(n, An);
    } else if(Bi) {
        m += 1;
        n = vec3_add_vec3(n, Bn);
    } else if(Ci) {
        m += 1;
        n = vec3_add_vec3(n, Cn);
    }

    if(m > 0) {
        *N = vec3_norm(n);
        return true;
    }

    // does not intersect
    *N = n;
    return false;
}

#define UNTITLED_FPS_PHYSICS_H

#endif //UNTITLED_FPS_PHYSICS_H
