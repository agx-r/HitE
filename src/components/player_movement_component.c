#include "player_movement_component.h"
#include "../core/logger.h"
#include "../core/physics_constants.h"
#include "component_registry.h"
#include "player_collider_component.h"
#include "shape_component.h"
#include "transform_component.h"

#include <float.h>
#include <math.h>
#include <string.h>

static vec3_t
vec3_make (float x, float y, float z)
{
  return (vec3_t){ x, y, z, 0.0f };
}

static vec3_t
vec3_add (vec3_t a, vec3_t b)
{
  return vec3_make (a.x + b.x, a.y + b.y, a.z + b.z);
}

static vec3_t
vec3_sub (vec3_t a, vec3_t b)
{
  return vec3_make (a.x - b.x, a.y - b.y, a.z - b.z);
}

static vec3_t
vec3_mul_scalar (vec3_t v, float s)
{
  return vec3_make (v.x * s, v.y * s, v.z * s);
}

static float
vec3_dot (vec3_t a, vec3_t b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float
vec3_length (vec3_t v)
{
  return sqrtf (vec3_dot (v, v));
}

static vec3_t
vec3_normalize (vec3_t v)
{
  float len = vec3_length (v);
  if (len < 1e-6f)
    return vec3_make (0.0f, 1.0f, 0.0f);
  return vec3_mul_scalar (v, 1.0f / len);
}

static void
move_input_callback (const event_t *event, void *user_data)
{
  if (!event || !user_data)
    return;

  player_movement_component_t *movement
      = (player_movement_component_t *)user_data;

  movement->input_direction = event->data.player_move.direction;
  movement->input_direction._padding = 0.0f;
  movement->jump_requested |= event->data.player_move.jump;
  movement->sprinting = event->data.player_move.sprint;
  movement->input_received = true;
}

static result_t
player_movement_component_start (ecs_world_t *world, entity_id_t entity,
                                 void *component_data)
{
  player_movement_component_t *movement
      = (player_movement_component_t *)component_data;

  memset (movement, 0, sizeof (*movement));
  movement->grounded = true;
  movement->velocity = (vec3_t){ 0.0f, 0.0f, 0.0f, 0.0f };
  movement->event_system
      = (event_system_t *)ecs_world_get_event_system (world);

  if (movement->event_system)
    {
      movement->move_listener = event_listen_entity (
          movement->event_system, EVENT_PLAYER_MOVE_INPUT, entity,
          move_input_callback, movement);
    }

  LOG_INFO ("PlayerMovement",
            "Player movement component started for entity %u", entity);

  return RESULT_SUCCESS;
}

static vec3_t
clamp_vec2 (vec3_t value, float max_length)
{
  float len_sq = value.x * value.x + value.z * value.z;
  if (len_sq <= max_length * max_length)
    return value;

  float len = sqrtf (len_sq);
  if (len < 0.0001f)
    return (vec3_t){ 0.0f, value.y, 0.0f, 0.0f };

  float scale = max_length / len;
  return (vec3_t){ value.x * scale, value.y, value.z * scale, 0.0f };
}

static void
apply_horizontal_motion (player_movement_component_t *movement,
                         float delta_time)
{
  float target_speed
      = PLAYER_MAX_GROUND_SPEED
        * (movement->sprinting ? PLAYER_SPRINT_MULTIPLIER : 1.0f);

  vec3_t horizontal
      = { movement->velocity.x, 0.0f, movement->velocity.z, 0.0f };

  if (movement->input_received)
    {
      vec3_t desired_velocity
          = { movement->input_direction.x * target_speed, 0.0f,
              movement->input_direction.z * target_speed, 0.0f };

      vec3_t delta = { desired_velocity.x - horizontal.x, 0.0f,
                       desired_velocity.z - horizontal.z, 0.0f };

      float accel = (movement->grounded ? PLAYER_GROUND_ACCELERATION
                                        : PLAYER_AIR_ACCELERATION)
                    * delta_time;

      vec3_t clamped = clamp_vec2 (delta, accel);
      horizontal.x += clamped.x;
      horizontal.z += clamped.z;
    }
  else
    {
      float damping
          = (movement->grounded ? PLAYER_GROUND_DAMPING : PLAYER_AIR_DAMPING)
            * delta_time;
      float factor = fmaxf (0.0f, 1.0f - damping);
      horizontal.x *= factor;
      horizontal.z *= factor;
    }

  movement->velocity.x = horizontal.x;
  movement->velocity.z = horizontal.z;
}

static void
integrate_vertical_motion (player_movement_component_t *movement,
                           float delta_time)
{
  if (movement->jump_requested && movement->grounded)
    {
      movement->velocity.y = PLAYER_JUMP_SPEED;
      movement->grounded = false;
    }

  movement->jump_requested = false;

  movement->velocity.y -= PLAYER_GRAVITY * delta_time;
  if (movement->velocity.y < -PLAYER_TERMINAL_VELOCITY)
    movement->velocity.y = -PLAYER_TERMINAL_VELOCITY;
}

typedef struct
{
  bool hit;
  float surface_distance;
  float penetration;
  vec3_t normal;
  vec3_t sample_point;
} sdf_collision_t;

typedef struct
{
  vec3_t position;
  vec3_t normal;
  bool hit;
  bool grounded;
} collision_result_t;

static float
player_collider_half_axis (const player_collider_component_t *collider)
{
  if (!collider)
    return 0.0f;

  float cylinder_height = collider->height - (2.0f * collider->radius);
  if (cylinder_height < 0.0f)
    cylinder_height = 0.0f;
  return 0.5f * cylinder_height;
}

static float
scene_sdf_at (component_array_t *shape_array, vec3_t point, float time_seconds)
{
  if (!shape_array || !shape_array->data)
    return FLT_MAX;

  float min_distance = FLT_MAX;
  size_t stride = shape_array->descriptor.data_size;

  for (size_t i = 0; i < shape_array->count; ++i)
    {
      if (!shape_array->active[i])
        continue;

      const shape_component_t *shape
          = (const shape_component_t *)((const char *)shape_array->data
                                        + i * stride);
      if (!shape->visible)
        continue;

      float distance = shape_evaluate_sdf (shape, point, time_seconds);
      if (distance < min_distance)
        min_distance = distance;
    }

  return min_distance;
}

static vec3_t
scene_sdf_normal (component_array_t *shape_array, vec3_t point,
                  float time_seconds)
{
  const float h = 0.01f;
  vec3_t offset = vec3_make (h, 0.0f, 0.0f);
  float dx
      = scene_sdf_at (shape_array, vec3_add (point, offset), time_seconds)
        - scene_sdf_at (shape_array, vec3_sub (point, offset), time_seconds);

  offset = vec3_make (0.0f, h, 0.0f);
  float dy
      = scene_sdf_at (shape_array, vec3_add (point, offset), time_seconds)
        - scene_sdf_at (shape_array, vec3_sub (point, offset), time_seconds);

  offset = vec3_make (0.0f, 0.0f, h);
  float dz
      = scene_sdf_at (shape_array, vec3_add (point, offset), time_seconds)
        - scene_sdf_at (shape_array, vec3_sub (point, offset), time_seconds);

  return vec3_normalize (vec3_make (dx, dy, dz));
}

static sdf_collision_t
detect_capsule_collision (component_array_t *shape_array, vec3_t position,
                          const player_collider_component_t *collider,
                          float time_seconds)
{
  sdf_collision_t result = { 0 };

  if (!collider)
    return result;

  vec3_t center = vec3_add (position, collider->offset);
  float half_axis = player_collider_half_axis (collider);

  float offsets[5] = { 0.0f };
  int sample_count = 1;
  if (half_axis > 1e-4f)
    {
      offsets[sample_count++] = half_axis;
      offsets[sample_count++] = -half_axis;
      offsets[sample_count++] = half_axis * 0.5f;
      offsets[sample_count++] = -half_axis * 0.5f;
    }

  float best_surface_distance = FLT_MAX;
  vec3_t best_sample = center;

  for (int i = 0; i < sample_count; ++i)
    {
      vec3_t sample = center;
      sample.y += offsets[i];

      float env_distance = scene_sdf_at (shape_array, sample, time_seconds);
      float surface_distance = env_distance - collider->radius;
      if (surface_distance < best_surface_distance)
        {
          best_surface_distance = surface_distance;
          best_sample = sample;
        }
    }

  float separation_target = collider->skin_width;
  if (best_surface_distance > separation_target)
    return result;

  result.hit = true;
  result.surface_distance = best_surface_distance;
  result.penetration = separation_target - best_surface_distance;
  result.sample_point = best_sample;
  result.normal = scene_sdf_normal (shape_array, best_sample, time_seconds);

  return result;
}

static collision_result_t
resolve_player_collisions (component_array_t *shape_array, vec3_t position,
                           player_collider_component_t *collider,
                           vec3_t *velocity, float time_seconds)
{
  collision_result_t result = { .position = position,
                                .normal = vec3_make (0.0f, 1.0f, 0.0f),
                                .hit = false,
                                .grounded = false };

  if (!collider || !shape_array)
    return result;

  const int max_iterations = 5;

  for (int i = 0; i < max_iterations; ++i)
    {
      sdf_collision_t collision = detect_capsule_collision (
          shape_array, position, collider, time_seconds);
      if (!collision.hit || collision.penetration <= 1e-5f)
        break;

      result.hit = true;
      result.normal = collision.normal;

      position = vec3_add (
          position, vec3_mul_scalar (collision.normal, collision.penetration));

      if (velocity)
        {
          float vn = vec3_dot (*velocity, collision.normal);
          if (vn < 0.0f)
            {
              velocity->x -= vn * collision.normal.x;
              velocity->y -= vn * collision.normal.y;
              velocity->z -= vn * collision.normal.z;
              velocity->_padding = 0.0f;
            }
        }

      if (collision.normal.y > 0.55f)
        {
          result.grounded = true;
          if (velocity && velocity->y < 0.0f)
            velocity->y = 0.0f;
        }
    }

  result.position = position;
  return result;
}

static result_t
player_movement_component_update (ecs_world_t *world, entity_id_t entity,
                                  void *component_data,
                                  const time_info_t *time)
{
  (void)entity;

  player_movement_component_t *movement
      = (player_movement_component_t *)component_data;

  if (!time)
    return RESULT_SUCCESS;

  float delta_time = time->delta_time;
  float time_seconds = (float)time->current_time;

  component_id_t collider_id = ecs_get_component_id (world, "player_collider");
  player_collider_component_t *collider = NULL;
  if (collider_id != INVALID_ENTITY)
    collider = (player_collider_component_t *)ecs_get_component (world, entity,
                                                                 collider_id);

  component_id_t transform_id = ecs_get_component_id (world, "transform");
  transform_component_t *transform = NULL;
  if (transform_id != INVALID_ENTITY)
    {
      transform = (transform_component_t *)ecs_get_component (world, entity,
                                                              transform_id);
    }

  component_array_t *shape_array = NULL;
  component_id_t shape_id = ecs_get_component_id (world, "shape");
  if (shape_id != INVALID_ENTITY && shape_id < world->component_count)
    {
      shape_array = &world->component_arrays[shape_id];
    }

  apply_horizontal_motion (movement, delta_time);
  integrate_vertical_motion (movement, delta_time);

  movement->grounded = false;

  if (transform)
    {
      vec3_t position = transform_get_position (transform);
      position.x += movement->velocity.x * delta_time;
      position.y += movement->velocity.y * delta_time;
      position.z += movement->velocity.z * delta_time;

      collision_result_t collision = resolve_player_collisions (
          shape_array, position, collider, &movement->velocity, time_seconds);

      position = collision.position;
      transform_set_position (transform, position);

      movement->grounded = collision.grounded;

      if (collider)
        {
          collider->grounded = collision.grounded;
          vec3_t normal = collision.hit ? collision.normal
                                        : vec3_make (0.0f, 1.0f, 0.0f);
          normal._padding = 0.0f;
          collider->surface_normal = normal;
        }
    }
  else if (collider)
    {
      collider->grounded = false;
      collider->surface_normal = vec3_make (0.0f, 1.0f, 0.0f);
      collider->surface_normal._padding = 0.0f;
    }

  movement->velocity._padding = 0.0f;

  movement->input_received = false;

  return RESULT_SUCCESS;
}

static void
player_movement_component_destroy (void *component_data)
{
  player_movement_component_t *movement
      = (player_movement_component_t *)component_data;

  if (!movement)
    return;

  if (movement->event_system && movement->move_listener)
    {
      event_unlisten (movement->event_system, movement->move_listener);
      movement->move_listener = 0;
    }
}

void
player_movement_component_register (ecs_world_t *world)
{
  static const char *dependencies[]
      = { "transform", "player_collider", "player_movement_controls", NULL };
  REGISTER_COMPONENT (
      world, "player_movement", player_movement_component_t,
      player_movement_component_start, player_movement_component_update, NULL,
      player_movement_component_destroy, "Player Movement", 64, dependencies);
}
