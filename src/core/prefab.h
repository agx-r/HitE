#ifndef HITE_PREFAB_H
#define HITE_PREFAB_H

#include "ecs.h"
#include "types.h"
#include <stdbool.h>
#include <stddef.h>

// Maximum component count per prefab
#define MAX_PREFAB_COMPONENTS 16

// Prefab component data
typedef struct
{
  const char *component_name;
  void *data;
  size_t data_size;
} prefab_component_data_t;

// Prefab definition
typedef struct
{
  const char *name;
  const char *description;

  // Components
  prefab_component_data_t components[MAX_PREFAB_COMPONENTS];
  size_t component_count;

  // Child prefabs (for hierarchical prefabs)
  const char **child_prefab_paths;
  size_t child_count;
} prefab_t;

// Prefab system for managing prefab definitions
typedef struct
{
  prefab_t *prefabs;
  size_t prefab_count;
  size_t prefab_capacity;
} prefab_system_t;

// Prefab system lifecycle
prefab_system_t *prefab_system_create (void);
void prefab_system_destroy (prefab_system_t *system);

// Load prefab from file (Scheme format)
result_t prefab_load (prefab_system_t *system, const char *filepath,
                      prefab_t **out_prefab);

// Load all prefabs from directory
result_t prefab_load_directory (prefab_system_t *system,
                                const char *directory_path);

// Find prefab by name
prefab_t *prefab_find (prefab_system_t *system, const char *name);

// Instantiate prefab into world
result_t prefab_instantiate (const prefab_t *prefab, ecs_world_t *world,
                             entity_id_t *out_entity);

// Save prefab to file (for editor)
result_t prefab_save (const prefab_t *prefab, const char *filepath);

// Create empty prefab
prefab_t *prefab_create (const char *name);

// Add component to prefab
result_t prefab_add_component (prefab_t *prefab, const char *component_name,
                               const void *data, size_t data_size);

// Cleanup prefab (free allocated data)
void prefab_cleanup (prefab_t *prefab);

#endif // HITE_PREFAB_H
