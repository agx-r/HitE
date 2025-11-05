#ifndef HITE_PREFAB_H
#define HITE_PREFAB_H

#include "ecs.h"
#include "types.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_PREFAB_COMPONENTS 16

typedef struct
{
  const char *component_name;
  void *data;
  size_t data_size;
} prefab_component_data_t;

typedef struct
{
  const char *name;
  const char *description;

  prefab_component_data_t components[MAX_PREFAB_COMPONENTS];
  size_t component_count;

  const char **child_prefab_paths;
  size_t child_count;
} prefab_t;

typedef struct
{
  prefab_t *prefabs;
  size_t prefab_count;
  size_t prefab_capacity;
  const char *prefabs_directory;
} prefab_system_t;

prefab_system_t *prefab_system_create (void);
void prefab_system_destroy (prefab_system_t *system);

void prefab_system_set_directory (prefab_system_t *system,
                                  const char *directory_path);

result_t prefab_load (prefab_system_t *system, const char *filepath,
                      prefab_t **out_prefab);

result_t prefab_load_directory (prefab_system_t *system,
                                const char *directory_path);

prefab_t *prefab_find (prefab_system_t *system, const char *name);

result_t prefab_instantiate (const prefab_t *prefab, ecs_world_t *world,
                             entity_id_t *out_entity);

result_t prefab_save (const prefab_t *prefab, const char *filepath);

prefab_t *prefab_create (const char *name);

result_t prefab_add_component (prefab_t *prefab, const char *component_name,
                               const void *data, size_t data_size);

void prefab_cleanup (prefab_t *prefab);

#endif
