#include "camera_movement_component.h"
#include "../core/events.h"
#include "../core/logger.h"
#include "camera_component.h"
#include "component_registry.h"

#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

camera_movement_component_t
camera_movement_create_default (void)
{
  camera_movement_component_t movement = { 0 };

  movement.move_speed = 5.0f;
  movement.enabled = true;
  memset (movement.keys, 0, sizeof (movement.keys));
  movement.key_press_listener = 0;
  movement.key_release_listener = 0;

  return movement;
}

static void
key_event_callback (const event_t *event, void *user_data)
{
  camera_movement_component_t *movement
      = (camera_movement_component_t *)user_data;

  if (!movement || !movement->enabled)
    return;

  int key = event->data.key.key;
  if (key >= 0 && key < 1024)
    {
      if (event->type == EVENT_KEY_PRESS)
        {
          movement->keys[key] = true;
        }
      else if (event->type == EVENT_KEY_RELEASE)
        {
          movement->keys[key] = false;
        }
    }
}

result_t
camera_movement_component_start (ecs_world_t *world, entity_id_t entity,
                                 void *component_data)
{
  camera_movement_component_t *movement
      = (camera_movement_component_t *)component_data;

  event_system_t *event_system
      = (event_system_t *)ecs_world_get_event_system (world);
  if (!event_system)
    {
      LOG_WARNING ("Camera Movement",
                   "No event system available, component will not receive key "
                   "events");
      return RESULT_SUCCESS;
    }

  movement->key_press_listener = event_listen (event_system, EVENT_KEY_PRESS,
                                               key_event_callback, movement);
  movement->key_release_listener = event_listen (
      event_system, EVENT_KEY_RELEASE, key_event_callback, movement);

  LOG_INFO ("Camera Movement", "Component started for entity %u", entity);

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

  right.x = -forward.z;
  right.y = 0;
  right.z = forward.x;

  float len = sqrtf (right.x * right.x + right.z * right.z);
  if (len > 0.0001f)
    {
      right.x /= len;
      right.z /= len;
    }

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
  camera_movement_component_t *movement
      = (camera_movement_component_t *)component_data;

  if (!movement)
    return;

  movement->key_press_listener = 0;
  movement->key_release_listener = 0;
}

void
camera_movement_component_register (ecs_world_t *world)
{
  REGISTER_COMPONENT (
      world, "camera_movement", camera_movement_component_t,
      camera_movement_component_start, camera_movement_component_update, NULL,
      camera_movement_component_destroy, "Camera Movement", 64, NULL);
}
