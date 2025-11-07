#include "camera_movement_component.h"
#include "../core/input_handler.h"
#include "../core/logger.h"
#include "camera_component.h"
#include "component_registry.h"

#include <GLFW/glfw3.h>
#include <math.h>
#include <string.h>

result_t
camera_movement_component_start (ecs_world_t *world, entity_id_t entity,
                                 void *component_data)
{
  (void)world;
  (void)entity;
  (void)component_data;

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

  input_handler_t *input_handler
      = (input_handler_t *)ecs_world_get_input_handler (world);
  if (!input_handler)
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
  vec3_t flat = { forward.x, 0.0f, forward.z };

  float len = sqrtf (flat.x * flat.x + flat.z * flat.z);
  if (len > 0.0001f)
    {
      flat.x /= len;
      flat.z /= len;
    }

  vec3_t right = { -flat.z, 0.0f, flat.x };
  vec3_t move = { 0.0f, 0.0f, 0.0f };

  if (input_handler_get_key_state (input_handler, GLFW_KEY_W))
    {
      move.x += flat.x;
      move.z += flat.z;
    }
  if (input_handler_get_key_state (input_handler, GLFW_KEY_S))
    {
      move.x -= flat.x;
      move.z -= flat.z;
    }
  if (input_handler_get_key_state (input_handler, GLFW_KEY_A))
    {
      move.x -= right.x;
      move.z -= right.z;
    }
  if (input_handler_get_key_state (input_handler, GLFW_KEY_D))
    {
      move.x += right.x;
      move.z += right.z;
    }
  if (input_handler_get_key_state (input_handler, GLFW_KEY_SPACE))
    {
      move.y += 1.0f;
    }
  if (input_handler_get_key_state (input_handler, GLFW_KEY_LEFT_SHIFT))
    {
      move.y -= 1.0f;
    }

  float move_len = sqrtf (move.x * move.x + move.y * move.y + move.z * move.z);
  if (move_len > 0.0001f)
    {
      move.x /= move_len;
      move.y /= move_len;
      move.z /= move_len;
    }

  camera->position.x += move.x * speed;
  camera->position.y += move.y * speed;
  camera->position.z += move.z * speed;

  return RESULT_SUCCESS;
}

void
camera_movement_component_destroy (void *component_data)
{
  (void)component_data;
}

void
camera_movement_component_register (ecs_world_t *world)
{
  REGISTER_COMPONENT (
      world, "camera_movement", camera_movement_component_t,
      camera_movement_component_start, camera_movement_component_update, NULL,
      camera_movement_component_destroy, "Camera Movement", 64, NULL);
}
