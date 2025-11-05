/*
 * Shape Interface Definition
 * 
 * This header enforces a unified structure for all shape implementations.
 * Each shape file MUST:
 * 1. Include only this header
 * 2. Implement ONLY the SDF function for that shape
 * 3. Follow the naming convention: sdf_<shapename>
 * 4. Use only vec3_t, vec2_t, float types
 * 5. Not include any other dependencies
 * 
 * DO NOT add any other functionality to shape files!
 */

#ifndef HITE_SHAPE_INTERFACE_H
#define HITE_SHAPE_INTERFACE_H

#include <math.h>

/* 
 * Forward declare vector types - actual definitions come from types.h
 * We do NOT redefine them here to avoid conflicts
 */
#ifndef HITE_TYPES_H
typedef struct { float x, y, z; float _padding; } vec3_t;
typedef struct { float x, y; float _padding[2]; } vec2_t;
#endif

/* Math helpers - shared by all shapes */
static inline float
vec3_length (vec3_t v)
{
  return sqrtf (v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline vec3_t
vec3_sub (vec3_t a, vec3_t b)
{
  return (vec3_t){ .x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z, ._padding = 0 };
}

static inline vec3_t
vec3_abs (vec3_t v)
{
  return (vec3_t){ .x = fabsf (v.x), .y = fabsf (v.y), .z = fabsf (v.z), ._padding = 0 };
}

static inline vec3_t
vec3_max_scalar (vec3_t v, float s)
{
  return (vec3_t){ .x = fmaxf (v.x, s), .y = fmaxf (v.y, s), .z = fmaxf (v.z, s), ._padding = 0 };
}

static inline float
vec3_max_component (vec3_t v)
{
  return fmaxf (v.x, fmaxf (v.y, v.z));
}

static inline float
vec3_dot (vec3_t a, vec3_t b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float
clampf (float x, float min, float max)
{
  return fmaxf (min, fminf (max, x));
}

#endif /* HITE_SHAPE_INTERFACE_H */
