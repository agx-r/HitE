#ifndef HITE_LIGHTING_COMPONENT_H
#define HITE_LIGHTING_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

typedef struct
{

  vec3_t sun_direction;

  vec3_t sun_color;

  float ambient_strength;

  float diffuse_strength;

  float shadow_bias;
  float shadow_softness;
  int shadow_steps;

  bool enabled;
} ALIGN_64 lighting_component_t;

void lighting_component_register (ecs_world_t *world);

lighting_component_t lighting_create_default (void);

lighting_component_t *lighting_find_on_camera (ecs_world_t *world,
                                               entity_id_t camera_entity);

#endif
