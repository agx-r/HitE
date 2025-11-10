#ifndef HITE_TRANSFORM_COMPONENT_H
#define HITE_TRANSFORM_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

typedef struct
{
  transform_t transform;
  bool dirty;
  float _padding[3];
} ALIGN_64 transform_component_t;

void transform_component_register (ecs_world_t *world);

vec3_t transform_get_position (const transform_component_t *transform);
void transform_set_position (transform_component_t *transform,
                             vec3_t position);

vec4_t transform_get_rotation (const transform_component_t *transform);
void transform_set_rotation (transform_component_t *transform,
                             vec4_t rotation);

vec3_t transform_forward (const transform_component_t *transform);
vec3_t transform_right (const transform_component_t *transform);
vec3_t transform_up (const transform_component_t *transform);

vec4_t transform_quaternion_from_euler (float pitch, float yaw, float roll);

#endif
