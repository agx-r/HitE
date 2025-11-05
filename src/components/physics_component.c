#include "physics_component.h"
#include "component_registry.h"
#include "shape_component.h"
#include <stdio.h>
#include <string.h>

#define GRAVITY -9.81f

static const char *physics_dependencies[] = { "shape", NULL };

result_t
physics_component_start (ecs_world_t *world, entity_id_t entity,
                         void *component_data)
{
  (void)world;
  (void)entity;

  physics_component_t *physics = (physics_component_t *)component_data;

  if (physics->mass == 0.0f)
    {
      physics->mass = 1.0f;
    }

  if (physics->drag == 0.0f)
    {
      physics->drag = 0.01f;
    }

  return RESULT_SUCCESS;
}

result_t
physics_component_update (ecs_world_t *world, entity_id_t entity,
                          void *component_data, const time_info_t *time)
{
  physics_component_t *physics = (physics_component_t *)component_data;

  if (physics->kinematic)
    {
      return RESULT_SUCCESS;
    }

  float dt = time->delta_time;

  if (physics->gravity_enabled)
    {
      physics->acceleration.y += GRAVITY;
    }

  physics->velocity.x += physics->acceleration.x * dt;
  physics->velocity.y += physics->acceleration.y * dt;
  physics->velocity.z += physics->acceleration.z * dt;

  float drag_factor = 1.0f - physics->drag * dt;
  if (drag_factor < 0.0f)
    drag_factor = 0.0f;

  physics->velocity.x *= drag_factor;
  physics->velocity.y *= drag_factor;
  physics->velocity.z *= drag_factor;

  component_id_t shape_id = ecs_get_component_id (world, "shape");
  if (shape_id != INVALID_ENTITY)
    {
      shape_component_t *shape
          = (shape_component_t *)ecs_get_component (world, entity, shape_id);
      if (shape)
        {
          shape->transform.position.x += physics->velocity.x * dt;
          shape->transform.position.y += physics->velocity.y * dt;
          shape->transform.position.z += physics->velocity.z * dt;

          shape->dirty = true;
        }
    }

  physics->acceleration = (vec3_t){ 0, 0, 0, 0 };

  return RESULT_SUCCESS;
}

void
physics_component_destroy (void *component_data)
{
  (void)component_data;
}

void
physics_component_register (ecs_world_t *world)
{
  REGISTER_COMPONENT (world, "physics", physics_component_t,
                      physics_component_start, physics_component_update, NULL,
                      physics_component_destroy, "Physics", 64,
                      physics_dependencies);
}

void
physics_apply_force (physics_component_t *physics, vec3_t force)
{
  if (!physics)
    return;

  float inv_mass = 1.0f / physics->mass;
  physics->acceleration.x += force.x * inv_mass;
  physics->acceleration.y += force.y * inv_mass;
  physics->acceleration.z += force.z * inv_mass;
}

void
physics_apply_impulse (physics_component_t *physics, vec3_t impulse)
{
  if (!physics)
    return;

  float inv_mass = 1.0f / physics->mass;
  physics->velocity.x += impulse.x * inv_mass;
  physics->velocity.y += impulse.y * inv_mass;
  physics->velocity.z += impulse.z * inv_mass;
}
