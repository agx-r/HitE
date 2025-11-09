#include "camera_rotation_component.h"
#include "../core/events.h"
#include "../core/logger.h"
#include "camera_component.h"
#include "component_registry.h"

#include <math.h>
#include <string.h>

static void
mouse_event_callback (const event_t *event, void *user_data)
{
  camera_rotation_component_t *rotation
      = (camera_rotation_component_t *)user_data;

  if (!rotation || !rotation->enabled || !rotation->mouse_captured)
    return;

  double xpos = (double)event->data.mouse.x;
  double ypos = (double)event->data.mouse.y;

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

  rotation->yaw += xoffset;
  rotation->pitch -= yoffset;

  if (rotation->pitch > rotation->max_pitch)
    rotation->pitch = rotation->max_pitch;
  if (rotation->pitch < rotation->min_pitch)
    rotation->pitch = rotation->min_pitch;
}

static result_t
camera_rotation_component_start (ecs_world_t *world, entity_id_t entity,
                                 void *component_data)
{
  camera_rotation_component_t *rotation
      = (camera_rotation_component_t *)component_data;

  event_system_t *event_system
      = (event_system_t *)ecs_world_get_event_system (world);
  if (!event_system)
    {
      LOG_WARNING (
          "Camera Rotation",
          "No event system available, component will not receive mouse "
          "events");
      return RESULT_SUCCESS;
    }

  rotation->mouse_move_listener = event_listen (
      event_system, EVENT_MOUSE_MOVE, mouse_event_callback, rotation);

  LOG_INFO ("Camera Rotation", "Component started for entity %u", entity);

  return RESULT_SUCCESS;
}

static result_t
camera_rotation_component_update (ecs_world_t *world, entity_id_t entity,
                                  void *component_data,
                                  const time_info_t *time)
{
  (void)time;

  camera_rotation_component_t *rotation
      = (camera_rotation_component_t *)component_data;

  if (!rotation->enabled)
    return RESULT_SUCCESS;

  component_id_t camera_id = ecs_get_component_id (world, "camera");
  if (camera_id == INVALID_ENTITY)
    return RESULT_SUCCESS;

  camera_component_t *camera
      = (camera_component_t *)ecs_get_component (world, entity, camera_id);
  if (camera)
    {

      camera->direction.x = cosf (rotation->yaw) * cosf (rotation->pitch);
      camera->direction.y = sinf (rotation->pitch);
      camera->direction.z = sinf (rotation->yaw) * cosf (rotation->pitch);

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

static void
camera_rotation_component_destroy (void *component_data)
{
  camera_rotation_component_t *rotation
      = (camera_rotation_component_t *)component_data;

  if (!rotation)
    return;

  rotation->mouse_move_listener = 0;
}

void
camera_rotation_component_register (ecs_world_t *world)
{
  REGISTER_COMPONENT (
      world, "camera_rotation", camera_rotation_component_t,
      camera_rotation_component_start, camera_rotation_component_update, NULL,
      camera_rotation_component_destroy, "Camera Rotation", 64, NULL);
}
