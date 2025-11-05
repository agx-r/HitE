#include "camera_movement_component.h"
#include "camera_component.h"

#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// Create default camera movement
camera_movement_component_t
camera_movement_create_default (void)
{
  camera_movement_component_t movement = { 0 };

  movement.move_speed = 5.0f;
  movement.enabled = true;
  memset (movement.keys, 0, sizeof (movement.keys));

  return movement;
}

// Process key input
void
camera_movement_process_key (camera_movement_component_t *movement, int key,
                              int action)
{
  if (!movement->enabled)
    return;

  if (key >= 0 && key < 1024)
    {
      if (action == GLFW_PRESS)
        movement->keys[key] = true;
      else if (action == GLFW_RELEASE)
        movement->keys[key] = false;
    }
}

// Component lifecycle
result_t
camera_movement_component_start (ecs_world_t *world, entity_id_t entity,
                                  void *component_data)
{
  (void)world;
  (void)entity;
  (void)component_data;

  printf ("[Camera Movement] Component started for entity %u\n", entity);

  return RESULT_SUCCESS;
}

result_t
camera_movement_component_update (ecs_world_t *world, entity_id_t entity,
                                   void *component_data,
                                   const time_info_t *time)
{
  camera_movement_component_t *movement
      = (camera_movement_component_t *)component_data;

  if (!movement->enabled)
    return RESULT_SUCCESS;

  // Get camera component
  component_id_t camera_id = ecs_get_component_id (world, "camera");
  if (camera_id == INVALID_ENTITY)
    return RESULT_SUCCESS;

  camera_component_t *camera
      = (camera_component_t *)ecs_get_component (world, entity, camera_id);
  if (!camera)
    return RESULT_SUCCESS;

  float speed = movement->move_speed * time->delta_time;

  vec3_t forward = camera->direction;
  vec3_t right;

  // Calculate right vector (perpendicular to forward and up)
  right.x = -forward.z;
  right.y = 0;
  right.z = forward.x;

  // Normalize right vector
  float len = sqrtf (right.x * right.x + right.z * right.z);
  if (len > 0.0001f)
    {
      right.x /= len;
      right.z /= len;
    }

  // Process input and update camera position
  if (movement->keys[GLFW_KEY_W])
    {
      camera->position.x += forward.x * speed;
      camera->position.y += forward.y * speed;
      camera->position.z += forward.z * speed;
    }
  if (movement->keys[GLFW_KEY_S])
    {
      camera->position.x -= forward.x * speed;
      camera->position.y -= forward.y * speed;
      camera->position.z -= forward.z * speed;
    }
  if (movement->keys[GLFW_KEY_A])
    {
      camera->position.x -= right.x * speed;
      camera->position.z -= right.z * speed;
    }
  if (movement->keys[GLFW_KEY_D])
    {
      camera->position.x += right.x * speed;
      camera->position.z += right.z * speed;
    }
  if (movement->keys[GLFW_KEY_SPACE])
    {
      camera->position.y += speed;
    }
  if (movement->keys[GLFW_KEY_LEFT_SHIFT])
    {
      camera->position.y -= speed;
    }

  return RESULT_SUCCESS;
}

void
camera_movement_component_destroy (void *component_data)
{
  (void)component_data;
}

// Register component
void
camera_movement_component_register (ecs_world_t *world)
{
  component_descriptor_t desc = { 0 };
  desc.name = "camera_movement";
  desc.data_size = sizeof (camera_movement_component_t);
  desc.alignment = 64;
  desc.start = camera_movement_component_start;
  desc.update = camera_movement_component_update;
  desc.destroy = camera_movement_component_destroy;

  component_id_t id;
  result_t result = ecs_register_component (world, &desc, &id);

  if (result.code == RESULT_OK)
    {
      printf ("[Camera Movement] Component registered (ID: %u)\n", id);
    }
  else
    {
      printf ("[Camera Movement] Failed to register: %s\n", result.message);
    }
}
