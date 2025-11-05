#include "camera_rotation_component.h"
#include "camera_component.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

// Create default camera rotation
camera_rotation_component_t
camera_rotation_create_default (float yaw, float pitch)
{
  camera_rotation_component_t rotation = { 0 };

  rotation.yaw = yaw;
  rotation.pitch = pitch;

  rotation.look_sensitivity = 0.003f;
  rotation.first_mouse = true;

  rotation.max_pitch = 1.5f;
  rotation.min_pitch = -1.5f;

  rotation.mouse_captured = true;
  rotation.enabled = true;

  return rotation;
}

// Process mouse movement
void
camera_rotation_process_mouse (camera_rotation_component_t *rotation,
                                double xpos, double ypos)
{
  if (!rotation->enabled || !rotation->mouse_captured)
    return;

  if (rotation->first_mouse)
    {
      rotation->last_mouse_x = xpos;
      rotation->last_mouse_y = ypos;
      rotation->first_mouse = false;
      return;
    }

  double xoffset = xpos - rotation->last_mouse_x;
  double yoffset = ypos - rotation->last_mouse_y;
  rotation->last_mouse_x = xpos;
  rotation->last_mouse_y = ypos;

  xoffset *= rotation->look_sensitivity;
  yoffset *= rotation->look_sensitivity;

  rotation->yaw += xoffset;  // Inverted X rotation
  rotation->pitch -= yoffset;

  // Constrain pitch
  if (rotation->pitch > rotation->max_pitch)
    rotation->pitch = rotation->max_pitch;
  if (rotation->pitch < rotation->min_pitch)
    rotation->pitch = rotation->min_pitch;
}

// Component lifecycle
result_t
camera_rotation_component_start (ecs_world_t *world, entity_id_t entity,
                                  void *component_data)
{
  (void)world;
  (void)entity;
  (void)component_data;

  printf ("[Camera Rotation] Component started for entity %u\n", entity);

  return RESULT_SUCCESS;
}

result_t
camera_rotation_component_update (ecs_world_t *world, entity_id_t entity,
                                   void *component_data,
                                   const time_info_t *time)
{
  (void)time;

  camera_rotation_component_t *rotation
      = (camera_rotation_component_t *)component_data;

  if (!rotation->enabled)
    return RESULT_SUCCESS;

  // Update camera component direction from rotation
  component_id_t camera_id = ecs_get_component_id (world, "camera");
  if (camera_id == INVALID_ENTITY)
    return RESULT_SUCCESS;

  camera_component_t *camera
      = (camera_component_t *)ecs_get_component (world, entity, camera_id);
  if (camera)
    {
      // Calculate direction from yaw and pitch
      camera->direction.x = cosf (rotation->yaw) * cosf (rotation->pitch);
      camera->direction.y = sinf (rotation->pitch);
      camera->direction.z = sinf (rotation->yaw) * cosf (rotation->pitch);

      // Normalize direction
      float len = sqrtf (camera->direction.x * camera->direction.x
                         + camera->direction.y * camera->direction.y
                         + camera->direction.z * camera->direction.z);
      if (len > 0.0001f)
        {
          camera->direction.x /= len;
          camera->direction.y /= len;
          camera->direction.z /= len;
        }
    }

  return RESULT_SUCCESS;
}

void
camera_rotation_component_destroy (void *component_data)
{
  (void)component_data;
}

// Register component
void
camera_rotation_component_register (ecs_world_t *world)
{
  component_descriptor_t desc = { 0 };
  desc.name = "camera_rotation";
  desc.data_size = sizeof (camera_rotation_component_t);
  desc.alignment = 64;
  desc.start = camera_rotation_component_start;
  desc.update = camera_rotation_component_update;
  desc.destroy = camera_rotation_component_destroy;

  component_id_t id;
  result_t result = ecs_register_component (world, &desc, &id);

  if (result.code == RESULT_OK)
    {
      printf ("[Camera Rotation] Component registered (ID: %u)\n", id);
    }
  else
    {
      printf ("[Camera Rotation] Failed to register: %s\n", result.message);
    }
}
