#ifndef HITE_CAMERA_MOVEMENT_COMPONENT_H
#define HITE_CAMERA_MOVEMENT_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

typedef struct
{

  float move_speed;

  bool enabled;
} ALIGN_64 camera_movement_component_t;

static result_t camera_movement_component_start (ecs_world_t *world,
                                                 entity_id_t entity,
                                                 void *component_data);
static result_t camera_movement_component_update (ecs_world_t *world,
                                                  entity_id_t entity,
                                                  void *component_data,
                                                  const time_info_t *time);
static void camera_movement_component_destroy (void *component_data);

void camera_movement_component_register (ecs_world_t *world);

camera_movement_component_t camera_movement_create_default (void);

#endif
