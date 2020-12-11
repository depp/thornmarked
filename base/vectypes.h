// Copyright 2014-2020 Dietrich Epp.
#pragma once

// 2D floating-point vector.
typedef struct vec2 {
    float v[2];
} vec2;

// 3D floating-point vector.
typedef struct vec3 {
    float v[3];
} vec3;

// Quaternion. Components are stored as (w, x, y, z). W is the real component.
typedef struct quat {
    float v[4];
} quat;

// 4x4 matrix.
typedef struct mat4 {
    float v[16];
} mat4;

// 2D integer vector.
typedef struct ivec2 {
    int v[2];
} ivec2;

// 3D integer vector.
typedef struct ivec3 {
    int v[3];
} ivec3;
