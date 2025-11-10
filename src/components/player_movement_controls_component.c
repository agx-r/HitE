#include "player_movement_controls_component.h"
#include "../core/events.h"
#include "../core/input_handler.h"
#include "../core/logger.h"
#include "camera_component.h"
#include "component_registry.h"
#include "transform_component.h"

#include <GLFW/glfw3.h>
#include <math.h>
#include <string.h>

static result_t
player_movement_controls_component_start (ecs_world_t *world,
                                          entity_id_t entity,
                                          void *component_data)
{
  (void)world;
  (void)entity;

  player_movement_controls_component_t *controls
      = (player_movement_controls_component_t *)component_data;

  memset (controls, 0, sizeof (*controls));
  controls->enabled = true;
  controls->auto_emit = true;
  controls->look_blend = 1.0f;

  LOG_INFO ("PlayerControls",
            "Player movement controls initialized for entity %u", entity);

  return RESULT_SUCCESS;
}

static vec3_t
calculate_move_direction (const transform_component_t *transform)
{
  if (!transform)
    return (vec3_t){ 0.0f, 0.0f, 0.0f, 0.0f };

  vec3_t forward = transform_forward (transform);
  vec3_t flat = { forward.x, 0.0f, forward.z, 0.0f };

  float len = sqrtf (flat.x * flat.x + flat.z * flat.z);
  if (len > 0.0001f)
    {
      flat.x /= len;
      flat.z /= len;
    }
  else
    {
      flat = (vec3_t){ 0.0f, 0.0f, 1.0f, 0.0f };
    }

  return flat;
}

static result_t
player_movement_controls_component_update (ecs_world_t *world,
                                           entity_id_t entity,
                                           void *component_data,
                                           const time_info_t *time)
{
  (void)time;

  player_movement_controls_component_t *controls
      = (player_movement_controls_component_t *)component_data;

  if (!controls->enabled)
    return RESULT_SUCCESS;

  input_handler_t *input_handler
      = (input_handler_t *)ecs_world_get_input_handler (world);
  event_system_t *event_system
      = (event_system_t *)ecs_world_get_event_system (world);

  if (!input_handler || !event_system)
    return RESULT_SUCCESS;

  component_id_t transform_id = ecs_get_component_id (world, "transform");
  transform_component_t *transform = NULL;
  if (transform_id != INVALID_ENTITY)
    {
      transform = (transform_component_t *)ecs_get_component (world, entity,
                                                              transform_id);
    }

  if (!transform)
    {
      entity_id_t camera_entity = INVALID_ENTITY;
      camera_component_t *camera = camera_find_active (world, &camera_entity);
      (void)camera;
      if (camera_entity != INVALID_ENTITY && transform_id != INVALID_ENTITY)
        {
          transform = (transform_component_t *)ecs_get_component (
              world, camera_entity, transform_id);
        }
    }

  vec3_t forward_flat = calculate_move_direction (transform);
  vec3_t right = { -forward_flat.z, 0.0f, forward_flat.x, 0.0f };
  vec3_t move = { 0.0f, 0.0f, 0.0f, 0.0f };

  if (input_handler_get_key_state (input_handler, GLFW_KEY_W))
    {
      move.x += forward_flat.x;
      move.z += forward_flat.z;
    }
  if (input_handler_get_key_state (input_handler, GLFW_KEY_S))
    {
      move.x -= forward_flat.x;
      move.z -= forward_flat.z;
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

  float length = sqrtf (move.x * move.x + move.z * move.z);
  if (length > 0.0001f)
    {
      move.x /= length;
      move.z /= length;
    }

  bool jump = input_handler_get_key_state (input_handler, GLFW_KEY_SPACE);
  bool sprint
      = input_handler_get_key_state (input_handler, GLFW_KEY_LEFT_SHIFT)
        || input_handler_get_key_state (input_handler, GLFW_KEY_RIGHT_SHIFT);

  event_t event = { 0 };
  event.type = EVENT_PLAYER_MOVE_INPUT;
  event.entity = entity;
  event.data.player_move.direction = move;
  event.data.player_move.direction._padding = 0.0f;
  event.data.player_move.jump = jump;
  event.data.player_move.sprint = sprint;

  result_t result = event_emit (event_system, &event);
  if (result.code != RESULT_OK)
    {
      LOG_WARNING ("PlayerControls", "Failed to emit move input event: %s",
                   result.message ? result.message : "unknown error");
    }

  return RESULT_SUCCESS;
}

static void
player_movement_controls_component_destroy (void *component_data)
{
  (void)component_data;
}

void
player_movement_controls_component_register (ecs_world_t *world)
{
  static const char *dependencies[] = { "transform", NULL };
  REGISTER_COMPONENT (world, "player_movement_controls",
                      player_movement_controls_component_t,
                      player_movement_controls_component_start,
                      player_movement_controls_component_update, NULL,
                      player_movement_controls_component_destroy,
                      "Player Controls", 64, dependencies);
}
