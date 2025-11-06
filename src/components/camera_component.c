#include "camera_component.h"
#include "../core/logger.h"
#include "component_registry.h"

#include <stdio.h>
#include <string.h>

camera_component_t
camera_create_default (vec3_t position, vec3_t direction)
{
  camera_component_t camera = { 0 };

  camera.position = position;
  camera.direction = direction;

  camera.up.x = 0;
  camera.up.y = 1;
  camera.up.z = 0;

  camera.fov = 70.0f;
  camera.near_plane = 0.1f;
  camera.far_plane = 1000.0f;

  camera.background_color = (vec3_t){ 0.06f, 0.06f, 0.06f, 0.0f };

  camera.is_active = true;

  return camera;
}

camera_component_t *
camera_find_active (ecs_world_t *world, entity_id_t *out_entity)
{
  if (!world)
    return NULL;

  component_id_t camera_id = ecs_get_component_id (world, "camera");
  if (camera_id == INVALID_ENTITY)
    return NULL;

  component_array_t *array = NULL;
  for (size_t i = 0; i < world->component_count; i++)
    {
      if (world->component_arrays[i].id == camera_id)
        {
          array = &world->component_arrays[i];
          break;
        }
    }

  if (!array)
    return NULL;

  for (size_t i = 0; i < array->count; i++)
    {
      if (!array->active[i])
        continue;

      camera_component_t *camera
          = (camera_component_t *)((char *)array->data
                                   + i * array->descriptor.data_size);
      if (camera->is_active)
        {
          if (out_entity)
            *out_entity = array->entities[i];
          return camera;
        }
    }

  return NULL;
}

result_t
camera_component_start (ecs_world_t *world, entity_id_t entity,
                        void *component_data)
{
  (void)world;
  (void)entity;

  camera_component_t *camera = (camera_component_t *)component_data;

  LOG_INFO ("Camera", "Camera component started for entity %u", entity);
  LOG_DEBUG ("Camera", "  Position: (%.2f, %.2f, %.2f)", camera->position.x,
             camera->position.y, camera->position.z);
  LOG_DEBUG ("Camera", "  Direction: (%.2f, %.2f, %.2f)", camera->direction.x,
             camera->direction.y, camera->direction.z);

  return RESULT_SUCCESS;
}

result_t
camera_component_update (ecs_world_t *world, entity_id_t entity,
                         void *component_data, const time_info_t *time)
{
  (void)world;
  (void)entity;
  (void)component_data;
  (void)time;

  return RESULT_SUCCESS;
}

result_t
camera_component_render (ecs_world_t *world, entity_id_t entity,
                         const void *component_data)
{
  (void)world;
  (void)entity;
  (void)component_data;

  return RESULT_SUCCESS;
}

void
camera_component_destroy (void *component_data)
{
  (void)component_data;
}

void
camera_component_register (ecs_world_t *world)
{
  REGISTER_COMPONENT (world, "camera", camera_component_t,
                      camera_component_start, camera_component_update,
                      camera_component_render, camera_component_destroy,
                      "Camera", 64, NULL);
}
