#include "transform_component.h"
#include "../core/logger.h"
#include "component_registry.h"

#include <math.h>
#include <string.h>

static vec4_t
quaternion_normalize (vec4_t q)
{
  float len = sqrtf (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  if (len < 0.000001f)
    {
      return (vec4_t){ 0.0f, 0.0f, 0.0f, 1.0f };
    }

  float inv_len = 1.0f / len;
  return (vec4_t){ q.x * inv_len, q.y * inv_len, q.z * inv_len,
                   q.w * inv_len };
}

static vec4_t
quaternion_multiply (vec4_t a, vec4_t b)
{
  return (vec4_t){ a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                   a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                   a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                   a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z };
}

static result_t
transform_component_start (ecs_world_t *world, entity_id_t entity,
                           void *component_data)
{
  (void)world;
  (void)entity;

  transform_component_t *transform = (transform_component_t *)component_data;

  if (!transform)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Transform component data is NULL");

  vec3_t initial_position = transform->transform.position;
  vec4_t initial_rotation = transform->transform.rotation;
  vec3_t initial_scale = transform->transform.scale;
  bool was_dirty = transform->dirty;

  memset (transform, 0, sizeof (*transform));

  transform->transform.position = initial_position;
  transform->transform.position._padding = 0.0f;

  float rot_len = sqrtf (initial_rotation.x * initial_rotation.x
                         + initial_rotation.y * initial_rotation.y
                         + initial_rotation.z * initial_rotation.z
                         + initial_rotation.w * initial_rotation.w);
  if (rot_len < 0.0001f)
    {
      initial_rotation = (vec4_t){ 0.0f, 0.0f, 0.0f, 1.0f };
    }
  transform->transform.rotation = quaternion_normalize (initial_rotation);

  if (fabsf (initial_scale.x) < 0.0001f && fabsf (initial_scale.y) < 0.0001f
      && fabsf (initial_scale.z) < 0.0001f)
    {
      initial_scale = (vec3_t){ 1.0f, 1.0f, 1.0f, 0.0f };
    }
  initial_scale._padding = 0.0f;
  transform->transform.scale = initial_scale;

  transform->dirty = was_dirty;

  LOG_INFO ("Transform", "Transform component started for entity %u", entity);

  return RESULT_SUCCESS;
}

static result_t
transform_component_update (ecs_world_t *world, entity_id_t entity,
                            void *component_data, const time_info_t *time)
{
  (void)world;
  (void)entity;
  (void)component_data;
  (void)time;
  return RESULT_SUCCESS;
}

static void
transform_component_destroy (void *component_data)
{
  (void)component_data;
}

void
transform_component_register (ecs_world_t *world)
{
  REGISTER_COMPONENT (world, "transform", transform_component_t,
                      transform_component_start, transform_component_update,
                      NULL, transform_component_destroy, "Transform", 64,
                      NULL);
}

vec3_t
transform_get_position (const transform_component_t *transform)
{
  if (!transform)
    return (vec3_t){ 666.0f, 666.0f, 666.0f, 0.0f };
  return transform->transform.position;
}

void
transform_set_position (transform_component_t *transform, vec3_t position)
{
  if (!transform)
    return;

  transform->transform.position
      = (vec3_t){ position.x, position.y, position.z, 0.0f };
  transform->dirty = true;
}

vec4_t
transform_get_rotation (const transform_component_t *transform)
{
  if (!transform)
    return (vec4_t){ 0.0f, 0.0f, 0.0f, 1.0f };

  return transform->transform.rotation;
}

void
transform_set_rotation (transform_component_t *transform, vec4_t rotation)
{
  if (!transform)
    return;

  transform->transform.rotation = quaternion_normalize (rotation);
  transform->dirty = true;
}

static vec3_t
quaternion_rotate_vector (vec4_t q, vec3_t v)
{
  vec4_t n = quaternion_normalize (q);
  float x = n.x;
  float y = n.y;
  float z = n.z;
  float w = n.w;

  vec3_t result
      = { (1.0f - 2.0f * (y * y + z * z)) * v.x
              + 2.0f * ((x * y - w * z) * v.y + (x * z + w * y) * v.z),
          2.0f
              * ((x * y + w * z) * v.x + (1.0f - 2.0f * (x * x + z * z)) * v.y
                 + (y * z - w * x) * v.z),
          2.0f
              * ((x * z - w * y) * v.x + (y * z + w * x) * v.y
                 + (1.0f - 2.0f * (x * x + y * y)) * v.z),
          0.0f };

  return result;
}

vec3_t
transform_forward (const transform_component_t *transform)
{
  if (!transform)
    return (vec3_t){ 0.0f, 0.0f, 1.0f, 0.0f };

  vec3_t forward = quaternion_rotate_vector (
      transform->transform.rotation, (vec3_t){ 0.0f, 0.0f, 1.0f, 0.0f });

  float len = sqrtf (forward.x * forward.x + forward.y * forward.y
                     + forward.z * forward.z);
  if (len > 0.0001f)
    {
      forward.x /= len;
      forward.y /= len;
      forward.z /= len;
    }

  forward._padding = 0.0f;
  return forward;
}

vec3_t
transform_right (const transform_component_t *transform)
{
  if (!transform)
    return (vec3_t){ 1.0f, 0.0f, 0.0f, 0.0f };

  vec3_t right = quaternion_rotate_vector (transform->transform.rotation,
                                           (vec3_t){ 1.0f, 0.0f, 0.0f, 0.0f });

  float len
      = sqrtf (right.x * right.x + right.y * right.y + right.z * right.z);
  if (len > 0.0001f)
    {
      right.x /= len;
      right.y /= len;
      right.z /= len;
    }

  right._padding = 0.0f;
  return right;
}

vec3_t
transform_up (const transform_component_t *transform)
{
  if (!transform)
    return (vec3_t){ 0.0f, 1.0f, 0.0f, 0.0f };

  vec3_t up = quaternion_rotate_vector (transform->transform.rotation,
                                        (vec3_t){ 0.0f, 1.0f, 0.0f, 0.0f });

  float len = sqrtf (up.x * up.x + up.y * up.y + up.z * up.z);
  if (len > 0.0001f)
    {
      up.x /= len;
      up.y /= len;
      up.z /= len;
    }

  up._padding = 0.0f;
  return up;
}

vec4_t
transform_quaternion_from_euler (float pitch, float yaw, float roll)
{
  float half_pitch = -pitch * 0.5f;
  float half_yaw = -yaw * 0.5f;
  float half_roll = -roll * 0.5f;

  vec4_t qpitch = { sinf (half_pitch), 0.0f, 0.0f, cosf (half_pitch) };
  vec4_t qyaw = { 0.0f, sinf (half_yaw), 0.0f, cosf (half_yaw) };
  vec4_t qroll = { 0.0f, 0.0f, sinf (half_roll), cosf (half_roll) };

  vec4_t q = quaternion_multiply (qyaw, quaternion_multiply (qpitch, qroll));
  return quaternion_normalize (q);
}
