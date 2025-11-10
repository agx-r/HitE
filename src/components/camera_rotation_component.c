#include "camera_rotation_component.h"
#include "../core/events.h"
#include "../core/logger.h"
#include "component_registry.h"
#include "transform_component.h"

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
      LOG_WARNING ("Camera Rotation", "No event system available, component "
                                      "will not receive mouse events");
      return RESULT_SUCCESS;
    }

  rotation->mouse_move_listener = event_listen (
      event_system, EVENT_MOUSE_MOVE, mouse_event_callback, rotation);
  LOG_INFO ("Camera Rotation", "Component started for entity %u", entity);
  return RESULT_SUCCESS;
}

static vec3_t
vec3_make (float x, float y, float z)
{
  return (vec3_t){ x, y, z, 0.0f };
}

static vec3_t
vec3_normalize (vec3_t v)
{
  float len = sqrtf (v.x * v.x + v.y * v.y + v.z * v.z);
  if (len < 0.0001f)
    return vec3_make (0.0f, 0.0f, 0.0f);
  float inv = 1.0f / len;
  return vec3_make (v.x * inv, v.y * inv, v.z * inv);
}

static vec4_t
quat_normalize (vec4_t q)
{
  float len = sqrtf (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  if (len < 0.0001f)
    return (vec4_t){ 0.0f, 0.0f, 0.0f, 1.0f };
  float inv = 1.0f / len;
  return (vec4_t){ q.x * inv, q.y * inv, q.z * inv, q.w * inv };
}

static vec4_t
quat_mul (vec4_t a, vec4_t b)
{
  vec4_t r;
  r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
  r.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
  r.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
  r.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
  return r;
}

static vec4_t
quat_from_axis_angle (vec3_t axis, float angle)
{
  axis = vec3_normalize (axis);
  float s = sinf (angle * 0.5f);
  float c = cosf (angle * 0.5f);
  return (vec4_t){ axis.x * s, axis.y * s, axis.z * s, c };
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

  component_id_t transform_id = ecs_get_component_id (world, "transform");
  if (transform_id == INVALID_ENTITY)
    return RESULT_SUCCESS;

  transform_component_t *transform
      = (transform_component_t *)ecs_get_component (world, entity,
                                                    transform_id);
  if (!transform)
    return RESULT_SUCCESS;

  float yaw = (float)rotation->yaw;
  float pitch = (float)rotation->pitch;

  vec3_t world_up = vec3_make (0.0f, 1.0f, 0.0f);

  vec4_t q_yaw = quat_from_axis_angle (world_up, yaw);

  vec4_t v = { 1.0f, 0.0f, 0.0f, 0.0f };
  vec4_t tmp = quat_mul (q_yaw, v);
  vec4_t qy_conj = (vec4_t){ -q_yaw.x, -q_yaw.y, -q_yaw.z, q_yaw.w };
  vec4_t rotated = quat_mul (tmp, qy_conj);
  vec3_t local_right = vec3_make (rotated.x, rotated.y, rotated.z);

  vec4_t q_pitch = quat_from_axis_angle (local_right, pitch);

  vec4_t q_total = quat_mul (q_pitch, q_yaw);
  q_total = quat_normalize (q_total);

  transform_set_rotation (transform, q_total);
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
  static const char *dependencies[] = { "transform", NULL };
  REGISTER_COMPONENT (
      world, "camera_rotation", camera_rotation_component_t,
      camera_rotation_component_start, camera_rotation_component_update, NULL,
      camera_rotation_component_destroy, "Camera Rotation", 64, dependencies);
}
