#ifndef HITE_WORLD_H
#define HITE_WORLD_H

#include "ecs.h"
#include "types.h"

// Component override for prefab instances
typedef struct
{
  const char *component_name;
  void *override_data; // Override data (NULL if not overriding)
  size_t override_data_size;
} component_override_t;

// Prefab instance with optional overrides
typedef struct
{
  const char *prefab_name;   // Reference to prefab to instantiate
  const char *instance_name; // Optional custom name for this instance

  // Component overrides (to modify prefab components)
  component_override_t *overrides;
  size_t override_count;

  // Additional components (not in prefab)
  component_override_t *additional_components;
  size_t additional_component_count;
} prefab_instance_t;

// Forward declare pointer from TinyScheme
typedef struct cell *pointer;

// Entity template (can be from prefab or inline)
typedef struct
{
  const char *name;
  const char *prefab_name; // If set, instantiate from this prefab

  // Components to add or override
  struct
  {
    const char *component_name;
    void *data; // Parsed component data (for new components)
    size_t data_size;
    pointer sexp; // Original S-expression (for partial overrides)
  } *components;
  size_t component_count;
} entity_template_t;

// World definition
typedef struct
{
  const char *name;

  // Fixed or variable time step
  float fixed_delta_time;
  bool use_fixed_timestep;

  // Prefab instances (with optional overrides)
  prefab_instance_t *prefab_instances;
  size_t prefab_instance_count;

  // Entity templates (defined inline in .world file)
  entity_template_t *entity_templates;
  size_t entity_template_count;
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

// Free world definition
void world_definition_free (world_definition_t *definition);

#endif // HITE_WORLD_H
