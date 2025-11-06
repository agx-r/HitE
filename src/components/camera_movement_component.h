#ifndef HITE_CAMERA_MOVEMENT_COMPONENT_H
#define HITE_CAMERA_MOVEMENT_COMPONENT_H

#include "../core/ecs.h"
#include "../core/events.h"
#include "../core/types.h"

typedef struct
{

  float move_speed;

  bool keys[1024];

  bool enabled;

  listener_id_t key_press_listener;
  listener_id_t key_release_listener;
} ALIGN_64 camera_movement_component_t;

result_t camera_movement_component_start (ecs_world_t *world,
                                          entity_id_t entity,
                                          void *component_data);
result_t camera_movement_component_update (ecs_world_t *world,
                                           entity_id_t entity,
                                           void *component_data,
                                           const time_info_t *time);
void camera_movement_component_destroy (void *component_data);

void camera_movement_component_register (ecs_world_t *world);

camera_movement_component_t camera_movement_create_default (void);

#endif
