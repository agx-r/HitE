#ifndef HITE_CAMERA_COMPONENT_H
#define HITE_CAMERA_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

typedef struct
{

  vec3_t position;
  vec3_t direction;
  vec3_t up;

  float fov;
  float near_plane;
  float far_plane;

  bool is_active;
} ALIGN_64 camera_component_t;

result_t camera_component_start (ecs_world_t *world, entity_id_t entity,
                                 void *component_data);
result_t camera_component_update (ecs_world_t *world, entity_id_t entity,
                                  void *component_data,
                                  const time_info_t *time);
result_t camera_component_render (ecs_world_t *world, entity_id_t entity,
                                  const void *component_data);
void camera_component_destroy (void *component_data);

void camera_component_register (ecs_world_t *world);

camera_component_t camera_create_default (vec3_t position, vec3_t direction);

camera_component_t *camera_find_active (ecs_world_t *world,
                                        entity_id_t *out_entity);

#endif
