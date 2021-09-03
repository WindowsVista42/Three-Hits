//
// Created by Windows Vista on 8/9/2021.
//

#ifndef UNTITLED_FPS_PHYSICS_H
#include "vmmath.h"

b32 triangle_normal_intersects_bottom_of_sphere(vec3 A, vec3 B, vec3 C, vec3 P, f32 rr, vec3* restrict N, f32* restrict d) {
    *N = vec3_norm(vec3_cross(vec3_sub_vec3(B, A), vec3_sub_vec3(C, A)));

    const f32 EPSILON = 0.000001;
    vec3 edge1, edge2, h, s, q;
    f32 a, f, u, v;

    edge1 = vec3_sub_vec3(B, A);
    edge2 = vec3_sub_vec3(C, A);
    h = vec3_cross(*N, edge2);
    a = vec3_dot(edge1, h);
    if(a > -EPSILON && a < EPSILON) { return false; }
    f = 1.0/a;
    s = vec3_sub_vec3(P, A);
    u = f * vec3_dot(s, h);
    if(u < 0.0 || u > 1.0) { return false; }
    q = vec3_cross(s, edge1);
    v = f * vec3_dot(*N, q);
    if(v < 0.0 || (u + v) > 1.0) { return false; }

    f32 t = f * vec3_dot(edge2, q);
    f32 tt = t * t;
    *d = rr - tt;
    if(tt < rr) { return true; }
    else { return false; }
}

b32 ray_intersects_triangle(vec3 A, vec3 B, vec3 C, vec3 S, vec3 E, vec3* restrict N, f32* restrict d) {
    *N = vec3_norm(vec3_cross(vec3_sub_vec3(B, A), vec3_sub_vec3(C, A)));
    vec3 R = vec3_norm(vec3_sub_vec3(E, S));

    const f32 EPSILON = 0.000001;
    vec3 edge1, edge2, h, s, q;
    f32 a, f, u, v;

    edge1 = vec3_sub_vec3(B, A);
    edge2 = vec3_sub_vec3(C, A);
    h = vec3_cross(R, edge2);
    a = vec3_dot(edge1, h);
    if(a > -EPSILON && a < EPSILON) { return false; }
    f = 1.0/a;
    s = vec3_sub_vec3(S, A);
    u = f * vec3_dot(s, h);
    if(u < 0.0 || u > 1.0) { return false; }
    q = vec3_cross(s, edge1);
    v = f * vec3_dot(R, q);
    if(v < 0.0 || (u + v) > 1.0) { return false; }

    f32 t = f * vec3_dot(edge2, q);
    f32 tt = t * t;
    *d = tt;
    if(t > EPSILON) { return true; }
    else { return false; }
}

b32 sphere_intersects_point(vec3 p, vec3 P, f32 rr, vec3* restrict N, f32* restrict d) {
    *N = vec3_norm(vec3_sub_vec3(P, p));
    *d = rr - vec3_distsq_vec3(P, p);
    return rr > vec3_distsq_vec3(P, p);
}

b32 line_intersects_sphere(vec3 A, vec3 B, vec3 P, f32 rr, vec3* restrict N, f32* restrict d) {
    f32 up0, up1, u;
    vec3 j, k;
    j = vec3_sub_vec3(P, A);
    k = vec3_sub_vec3(B, A);
    up0 = vec3_dot(j, k);
    up1 = vec3_dot(k, k);
    u = up0 / up1;
    if(u < 0.0 || u > 1.0) { return false; }

    vec3 iP = vec3_add_vec3(A, vec3_mul_f32(vec3_sub_vec3(B, A), u));
    b32 intersects = sphere_intersects_point(iP, P, rr, N, d);
    *d = vec3_distsq_vec3(iP, A);
    return intersects;
}

b32 line_intersects_sphere_incorrect(vec3 A, vec3 B, vec3 P, f32 rr, vec3* restrict N, f32* restrict d) {
    f32 up0, up1, u;
    vec3 j, k;
    j = vec3_sub_vec3(P, A);
    k = vec3_sub_vec3(B, A);
    up0 = vec3_dot(j, k);
    up1 = vec3_dot(k, k);
    u = up0 / up1;
    if(u < 0.0 || u > 1.0) { return false; }

    vec3 iP = vec3_add_vec3(A, vec3_mul_f32(vec3_sub_vec3(B, A), u));
    return sphere_intersects_point(iP, P, rr, N, d);
}

//TODO(sean): get this to work with vertex - index data
//TODO(sean): move this to just use re-duped vertex data
//TODO(sean): line intersection
//TODO(sean): properly optimize this in the future
b32 sphere_collides_with_triangle(vec3 A, vec3 B, vec3 C, vec3 P, f32 r, vec3* restrict N, f32* restrict d) {
    f32 rr = r * r;
    vec3 Rn, ABn, BCn, CAn, An, Bn, Cn;
    f32 Rd, ABd, BCd, CAd, Ad, Bd, Cd;
    b32 Ri, ABi, BCi, CAi, Ai, Bi, Ci;

    Ri = triangle_normal_intersects_bottom_of_sphere(A, B, C, P, rr, &Rn, &Rd);
    ABi = line_intersects_sphere_incorrect(A, B, P, rr, &ABn, &ABd);
    BCi = line_intersects_sphere_incorrect(B, C, P, rr, &BCn, &BCd);
    CAi = line_intersects_sphere_incorrect(C, A, P, rr, &CAn, &CAd);
    Ai = sphere_intersects_point(A, P, rr, &An, &Ad);
    Bi = sphere_intersects_point(B, P, rr, &Bn, &Bd);
    Ci = sphere_intersects_point(C, P, rr, &Cn, &Cd);

    vec3 closest_n = VEC3_ZERO;
    f32 closest_d = 0.0f;

    // intersects with top
    if(Ri) {
        *N = Rn;
        *d = Rd;
        return true;
    }

    if(ABi && ABd > closest_d) {
        closest_n = ABn;
        closest_d = ABd;
    }
    if(BCi && BCd > closest_d) {
        closest_n = BCn;
        closest_d = BCd;
    }
    if(CAi && CAd > closest_d) {
        closest_n = CAn;
        closest_d = CAd;
    }
    if(Ai && Ad > closest_d) {
        closest_n = An;
        closest_d = Ad;
    }
    if(Bi && Bd > closest_d) {
        closest_n = Bn;
        closest_d = Bd;
    }
    if(Ci && Cd > closest_d) {
        closest_n = Cn;
        closest_d = Cd;
    }

    if(closest_d != 0.0f) {
        *N = closest_n;
        *d = closest_d;
        return true;
    } else {
        *N = VEC3_ZERO;
        *d = 0.0;
        return false;
    }
}

u32 player_ray_intersects_enemy_list(EnemyList* enemies, vec3 P, vec3 E, f32 wall_distance) {
    f32 best_enemy_distance = FLT_MAX;
    u32 hit_index = UINT_MAX;

    for (usize index = 0; index < enemies->entities.length; index += 1) {
        vec3 Pe = *(vec3 *) &enemies->entities.position_rotations[index].x;
        vec3 enemyN;
        f32 enemy_distance;
        f32 r = 1.8;
        f32 rr = r * r;

        // we know that we intersect the enemy
        if (line_intersects_sphere(P, E, Pe, rr, &enemyN, &enemy_distance) == true) {
            if (enemy_distance < wall_distance && enemy_distance < best_enemy_distance) {
                best_enemy_distance = enemy_distance;
                hit_index = index;
            }
        }
    }

    return hit_index;
}


#define UNTITLED_FPS_PHYSICS_H

#endif //UNTITLED_FPS_PHYSICS_H
