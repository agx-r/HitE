#ifndef HITE_CAMERA_COMPONENT_H
#define HITE_CAMERA_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

typedef struct
{

  float fov;
  float near_plane;
  float far_plane;

  vec3_t background_color;

  bool is_active;
} ALIGN_64 camera_component_t;

void camera_component_register (ecs_world_t *world);

camera_component_t *camera_find_active (ecs_world_t *world,
                                        entity_id_t *out_entity);

#define CAMERA_DEFAULT_FOV 70.0f

#endif
