#ifndef HITE_WORLD_H
#define HITE_WORLD_H

#include "component_parsers.h"
#include "ecs.h"
#include "prefab.h"
#include "types.h"

typedef struct
{
  const char *component_name;
  void *override_data;
  size_t override_data_size;
} component_override_t;

typedef struct
{
  const char *prefab_name;
  const char *instance_name;

  component_override_t *overrides;
  size_t override_count;

  component_override_t *additional_components;
  size_t additional_component_count;
} prefab_instance_t;

typedef struct cell *pointer;

typedef struct
{
  const char *name;
  const char *prefab_name;

  struct
  {
    const char *component_name;
    void *data;
    size_t data_size;
    pointer sexp;
  } *components;
  size_t component_count;
} entity_template_t;

typedef struct
{
  const char *name;

  float fixed_delta_time;
  bool use_fixed_timestep;

  prefab_instance_t *prefab_instances;
  size_t prefab_instance_count;

  entity_template_t *entity_templates;
  size_t entity_template_count;
} world_definition_t;

typedef struct
{
  ecs_world_t *active_world;
  world_definition_t *current_definition;

  world_definition_t **loaded_worlds;
  size_t loaded_world_count;
} world_manager_t;

world_manager_t *world_manager_create (void);
void world_manager_destroy (world_manager_t *manager);

result_t world_load (world_manager_t *manager,
                     const world_definition_t *definition,
                     prefab_system_t *prefab_system);

result_t world_instantiate_templates (ecs_world_t *world,
                                      const entity_template_t *templates,
                                      size_t count,
                                      prefab_system_t *prefab_system);

result_t world_switch (world_manager_t *manager,
                       const world_definition_t *definition,
                       prefab_system_t *prefab_system);

result_t world_update (world_manager_t *manager, float delta_time);

void world_definition_free (world_definition_t *definition);

#endif
