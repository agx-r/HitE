#include "camera_component.h"
#include "../core/logger.h"
#include "component_registry.h"
#include "transform_component.h"

#include <stdio.h>
#include <string.h>

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

static result_t
camera_component_start (ecs_world_t *world, entity_id_t entity,
                        void *component_data)
{
  (void)world;
  (void)entity;

  camera_component_t *camera = (camera_component_t *)component_data;

  component_id_t transform_id = ecs_get_component_id (world, "transform");
  transform_component_t *transform
      = (transform_component_t *)ecs_get_component (world, entity,
                                                    transform_id);

  vec3_t position = transform ? transform_get_position (transform)
                              : (vec3_t){ 0.0f, 0.0f, 0.0f, 0.0f };
  vec3_t forward = transform ? transform_forward (transform)
                             : (vec3_t){ 0.0f, 0.0f, 1.0f, 0.0f };

  LOG_INFO ("Camera", "Camera component started for entity %u", entity);
  LOG_DEBUG ("Camera", "  Position: (%.2f, %.2f, %.2f)", position.x,
             position.y, position.z);
  LOG_DEBUG ("Camera", "  Forward: (%.2f, %.2f, %.2f)", forward.x, forward.y,
             forward.z);

  return RESULT_SUCCESS;
}

static result_t
camera_component_update (ecs_world_t *world, entity_id_t entity,
                         void *component_data, const time_info_t *time)
{
  (void)world;
  (void)entity;
  (void)component_data;
  (void)time;

  return RESULT_SUCCESS;
}

static result_t
camera_component_render (ecs_world_t *world, entity_id_t entity,
                         const void *component_data)
{
  (void)world;
  (void)entity;
  (void)component_data;

  return RESULT_SUCCESS;
}

static void
camera_component_destroy (void *component_data)
{
  (void)component_data;
}

void
camera_component_register (ecs_world_t *world)
{
  static const char *dependencies[] = { "transform", NULL };
  REGISTER_COMPONENT (world, "camera", camera_component_t,
                      camera_component_start, camera_component_update,
                      camera_component_render, camera_component_destroy,
                      "Camera", 64, dependencies);
}
