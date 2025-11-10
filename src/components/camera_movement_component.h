#ifndef HITE_CAMERA_MOVEMENT_COMPONENT_H
#define HITE_CAMERA_MOVEMENT_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

typedef struct
{

  float move_speed;

  bool enabled;
} ALIGN_64 camera_movement_component_t;

void camera_movement_component_register (ecs_world_t *world);

camera_movement_component_t camera_movement_create_default (void);

#endif
