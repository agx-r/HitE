#ifndef HITE_WORLD_H
#define HITE_WORLD_H

#include "ecs.h"
#include "types.h"

// Entity template for world definition
typedef struct
{
  const char *name;

  // Components to add
  struct
  {
    const char *component_name;
    const void *initial_data;
  } *components;
  size_t component_count;
} entity_template_t;

// World generation function
typedef result_t (*world_generator_fn) (ecs_world_t *world, void *user_data);

// World definition
typedef struct
{
  const char *name;

  // Fixed or variable time step
  float fixed_delta_time;
  bool use_fixed_timestep;

  // Entity templates
  entity_template_t *entity_templates;
  size_t entity_template_count;

  // Optional: procedural generation
  world_generator_fn generator;
  void *generator_data;

  // GPU generation flag
  bool generate_on_gpu;
} world_definition_t;

// Global world manager
typedef struct
{
  ecs_world_t *active_world;
  world_definition_t *current_definition;

  // World switching
  world_definition_t **loaded_worlds;
  size_t loaded_world_count;
} world_manager_t;

// World management functions
world_manager_t *world_manager_create (void);
void world_manager_destroy (world_manager_t *manager);

// Load world from definition
result_t world_load (world_manager_t *manager,
                     const world_definition_t *definition);

// Instantiate entities from templates
result_t world_instantiate_templates (ecs_world_t *world,
                                      const entity_template_t *templates,
                                      size_t count);

// Switch active world at runtime
result_t world_switch (world_manager_t *manager,
                       const world_definition_t *definition);

// Update active world
result_t world_update (world_manager_t *manager, float delta_time);

#endif // HITE_WORLD_H
