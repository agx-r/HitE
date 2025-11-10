#include "player_collider_component.h"
#include "../core/logger.h"
#include "component_registry.h"

#include <math.h>
#include <string.h>

void
player_collider_recalculate_offset (player_collider_component_t *collider)
{
  if (!collider)
    return;

  if (collider->radius < 0.001f)
    collider->radius = 0.001f;

  float min_height = collider->radius * 2.0f;
  if (collider->height < min_height)
    collider->height = min_height;

  if (collider->camera_height < collider->radius)
    collider->camera_height = collider->radius;

  float cylinder_height = collider->height - 2.0f * collider->radius;
  if (cylinder_height < 0.0f)
    cylinder_height = 0.0f;

  float half_axis = 0.5f * cylinder_height;
  float center_offset_y
      = -collider->camera_height + collider->radius + half_axis;

  collider->offset = (vec3_t){ 0.0f, center_offset_y, 0.0f, 0.0f };
}

static result_t
player_collider_component_start (ecs_world_t *world, entity_id_t entity,
                                 void *component_data)
{
  (void)world;
  (void)entity;

  player_collider_component_t *collider
      = (player_collider_component_t *)component_data;

  if (!collider)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Player collider data is NULL");

  if (collider->radius < 0.001f)
    collider->radius = 0.35f;
  if (collider->height < collider->radius * 2.0f)
    collider->height = fmaxf (collider->radius * 2.0f, 1.8f);
  if (collider->skin_width < 0.0f)
    collider->skin_width = 0.0f;
  if (collider->skin_width > collider->radius)
    collider->skin_width = collider->radius * 0.5f;
  if (collider->camera_height < collider->radius)
    collider->camera_height = collider->radius + 1.0f;

  player_collider_recalculate_offset (collider);

  float normal_len
      = sqrtf (collider->surface_normal.x * collider->surface_normal.x
               + collider->surface_normal.y * collider->surface_normal.y
               + collider->surface_normal.z * collider->surface_normal.z);
  if (normal_len < 0.0001f)
    collider->surface_normal = (vec3_t){ 0.0f, 1.0f, 0.0f, 0.0f };
  else
    {
      collider->surface_normal.x /= normal_len;
      collider->surface_normal.y /= normal_len;
      collider->surface_normal.z /= normal_len;
      collider->surface_normal._padding = 0.0f;
    }

  collider->grounded = collider->grounded;

  LOG_INFO ("PlayerCollider", "Player collider initialized (r=%.2f, h=%.2f)",
            collider->radius, collider->height);

  return RESULT_SUCCESS;
}

static result_t
player_collider_component_update (ecs_world_t *world, entity_id_t entity,
                                  void *component_data,
                                  const time_info_t *time)
{
  (void)world;
  (void)entity;
  (void)component_data;
  (void)time;
  return RESULT_SUCCESS;
}

static void
player_collider_component_destroy (void *component_data)
{
  (void)component_data;
}

void
player_collider_component_register (ecs_world_t *world)
{
  REGISTER_COMPONENT (
      world, "player_collider", player_collider_component_t,
      player_collider_component_start, player_collider_component_update, NULL,
      player_collider_component_destroy, "Player Collider", 64, NULL);
}
