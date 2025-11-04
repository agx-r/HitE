#ifndef HITE_PHYSICS_COMPONENT_H
#define HITE_PHYSICS_COMPONENT_H

#include "../core/ecs.h"
#include "../core/events.h"
#include "../core/types.h"

// Physics component data
typedef struct
{
  vec3_t velocity;
  vec3_t acceleration;
  float mass;
  float drag;

  // Collision
  bool collider_enabled;
  float collision_radius;

  // Flags
  bool kinematic; // Not affected by physics
  bool gravity_enabled;
} ALIGN_64 physics_component_t;

// Component lifecycle
result_t physics_component_start (ecs_world_t *world, entity_id_t entity,
                                  void *component_data);
result_t physics_component_update (ecs_world_t *world, entity_id_t entity,
                                   void *component_data,
                                   const time_info_t *time);
void physics_component_destroy (void *component_data);

// Register component
void physics_component_register (ecs_world_t *world);

// Physics helpers
void physics_apply_force (physics_component_t *physics, vec3_t force);
void physics_apply_impulse (physics_component_t *physics, vec3_t impulse);

#endif // HITE_PHYSICS_COMPONENT_H
