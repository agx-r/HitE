#ifndef HITE_PLAYER_MOVEMENT_COMPONENT_H
#define HITE_PLAYER_MOVEMENT_COMPONENT_H

#include "../core/ecs.h"
#include "../core/events.h"
#include "../core/types.h"

typedef struct
{
  vec3_t velocity;
  vec3_t input_direction;
  bool jump_requested;
  bool sprinting;
  bool grounded;
  bool input_received;
  listener_id_t move_listener;
  event_system_t *event_system;
} ALIGN_64 player_movement_component_t;

void player_movement_component_register (ecs_world_t *world);

#endif
