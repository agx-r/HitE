#include "camera_component.h"

#include <stdio.h>
#include <string.h>

// Create camera with default settings
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

  camera.is_active = true;

  return camera;
}

// Find active camera in world
camera_component_t *
camera_find_active (ecs_world_t *world, entity_id_t *out_entity)
{
  if (!world)
    return NULL;

  component_id_t camera_id = ecs_get_component_id (world, "camera");
  if (camera_id == INVALID_ENTITY)
    return NULL;

  // Iterate through all camera components to find active one
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

// Component lifecycle
result_t
camera_component_start (ecs_world_t *world, entity_id_t entity,
                        void *component_data)
{
  (void)world;
  (void)entity;

  camera_component_t *camera = (camera_component_t *)component_data;

  printf ("[Camera] Camera component started for entity %u\n", entity);
  printf ("  Position: (%.2f, %.2f, %.2f)\n", camera->position.x,
          camera->position.y, camera->position.z);
  printf ("  Direction: (%.2f, %.2f, %.2f)\n", camera->direction.x,
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

  // Camera component is now just data, no update logic needed
  // Movement and rotation are handled by separate components

  return RESULT_SUCCESS;
}

result_t
camera_component_render (ecs_world_t *world, entity_id_t entity,
                         const void *component_data)
{
  (void)world;
  (void)entity;
  (void)component_data;

  // Camera rendering is handled by render system
  // This is just a placeholder for future extensions

  return RESULT_SUCCESS;
}

void
camera_component_destroy (void *component_data)
{
  (void)component_data;
  // No dynamic allocations to clean up
}

// Register component
void
camera_component_register (ecs_world_t *world)
{
  component_descriptor_t desc = { 0 };
  desc.name = "camera";
  desc.data_size = sizeof (camera_component_t);
  desc.alignment = 64;
  desc.start = camera_component_start;
  desc.update = camera_component_update;
  desc.render = camera_component_render;
  desc.destroy = camera_component_destroy;

  component_id_t id;
  result_t result = ecs_register_component (world, &desc, &id);

  if (result.code == RESULT_OK)
    {
      printf ("[Camera] Camera component registered (ID: %u)\n", id);
    }
  else
    {
      printf ("[Camera] Failed to register camera component: %s\n",
              result.message);
    }
}
