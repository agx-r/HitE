#ifndef HITE_PLAYER_COMPONENT_H
#define HITE_PLAYER_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

typedef struct
{
  entity_id_t camera_entity;
  bool active;
} ALIGN_64 player_component_t;

void player_component_register (ecs_world_t *world);

#endif
