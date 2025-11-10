#ifndef HITE_PLAYER_COLLIDER_COMPONENT_H
#define HITE_PLAYER_COLLIDER_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

typedef struct
{
  float radius;
  float height;
  float skin_width;
  float camera_height;
  vec3_t offset;
  vec3_t surface_normal;
  bool grounded;
  float _padding;
} ALIGN_64 player_collider_component_t;

void
player_collider_recalculate_offset (player_collider_component_t *collider);

void player_collider_component_register (ecs_world_t *world);

#endif
