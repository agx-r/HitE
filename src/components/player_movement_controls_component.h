#ifndef HITE_PLAYER_MOVEMENT_CONTROLS_COMPONENT_H
#define HITE_PLAYER_MOVEMENT_CONTROLS_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

typedef struct
{
  bool enabled;
  bool auto_emit;
  float look_blend;
  float _padding;
} ALIGN_64 player_movement_controls_component_t;

void player_movement_controls_component_register (ecs_world_t *world);

#endif
