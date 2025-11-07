#include "lighting_component.h"
#include "component_registry.h"

#include <math.h>
#include <string.h>

lighting_component_t
lighting_create_default (void)
{
  lighting_component_t lighting = { 0 };

  lighting.sun_direction.x = 0.3f;
  lighting.sun_direction.y = 0.8f;
  lighting.sun_direction.z = 0.5f;

  float len = sqrtf (lighting.sun_direction.x * lighting.sun_direction.x
                     + lighting.sun_direction.y * lighting.sun_direction.y
                     + lighting.sun_direction.z * lighting.sun_direction.z);
  lighting.sun_direction.x /= len;
  lighting.sun_direction.y /= len;
  lighting.sun_direction.z /= len;

  lighting.sun_color.x = 1.0f;
  lighting.sun_color.y = 0.95f;
  lighting.sun_color.z = 0.8f;

  lighting.ambient_strength = 0.2f;
  lighting.diffuse_strength = 0.8f;

  lighting.shadow_bias = 0.01f;
  lighting.shadow_softness = 0.5f;
  lighting.shadow_steps = 32;

  lighting.enabled = true;

  return lighting;
}

lighting_component_t *
lighting_find_on_camera (ecs_world_t *world, entity_id_t camera_entity)
{
  if (!world || camera_entity == INVALID_ENTITY)
    return NULL;

  component_id_t lighting_id = ecs_get_component_id (world, "lighting");
  if (lighting_id == INVALID_ENTITY)
    return NULL;

  if (!ecs_has_component (world, camera_entity, lighting_id))
    return NULL;

  return (lighting_component_t *)ecs_get_component (world, camera_entity,
                                                    lighting_id);
}

result_t
lighting_component_start (ecs_world_t *world, entity_id_t entity,
                          void *component_data)
{
  (void)world;
  (void)entity;

  (void)component_data;

  return RESULT_SUCCESS;
}

result_t
lighting_component_update (ecs_world_t *world, entity_id_t entity,
                           void *component_data, const time_info_t *time)
{
  (void)world;
  (void)entity;
  (void)component_data;
  (void)time;

  return RESULT_SUCCESS;
}

result_t
lighting_component_render (ecs_world_t *world, entity_id_t entity,
                           const void *component_data)
{
  (void)world;
  (void)entity;
  (void)component_data;

  return RESULT_SUCCESS;
}

void
lighting_component_destroy (void *component_data)
{
  (void)component_data;
}

void
lighting_component_register (ecs_world_t *world)
{

  static const char *dependencies[] = { "camera", NULL };

  REGISTER_COMPONENT (world, "lighting", lighting_component_t,
                      lighting_component_start, lighting_component_update,
                      lighting_component_render, lighting_component_destroy,
                      "Lighting", 64, dependencies);
}
