#ifndef HITE_RESOURCES_H
#define HITE_RESOURCES_H

#include "types.h"
#include <time.h>

#define MAX_RESOURCES 1024

typedef enum
{
  RESOURCE_SHADER,
  RESOURCE_TEXTURE,
  RESOURCE_TABLE,
  RESOURCE_NOISE,
} resource_type_t;

typedef struct
{
  resource_id_t id;
  resource_type_t type;
  void *data;
  size_t size;

  char *file_path;
  time_t last_modified;

  uint32_t ref_count;
} resource_t;

typedef struct
{
  resource_t *resources;
  size_t resource_count;
  size_t resource_capacity;

  resource_id_t next_id;

  struct
  {
    resource_id_t *ids;
    const char **paths;
    size_t count;
  } path_cache;
} resource_manager_t;

resource_manager_t *resource_manager_create (void);
void resource_manager_destroy (resource_manager_t *manager);

result_t resource_load (resource_manager_t *manager, const char *path,
                        resource_type_t type, resource_id_t *out_id);

resource_t *resource_get (resource_manager_t *manager, resource_id_t id);
resource_t *resource_get_by_path (resource_manager_t *manager,
                                  const char *path);

void resource_acquire (resource_manager_t *manager, resource_id_t id);
void resource_release (resource_manager_t *manager, resource_id_t id);

result_t resource_check_reload (resource_manager_t *manager);
result_t resource_reload (resource_manager_t *manager, resource_id_t id);

#endif
