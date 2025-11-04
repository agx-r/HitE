#include "resources.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

resource_manager_t *
resource_manager_create (void)
{
  resource_manager_t *manager = calloc (1, sizeof (resource_manager_t));
  if (!manager)
    return NULL;

  manager->resource_capacity = MAX_RESOURCES;
  manager->resources
      = calloc (manager->resource_capacity, sizeof (resource_t));
  manager->path_cache.ids = calloc (MAX_RESOURCES, sizeof (resource_id_t));
  manager->path_cache.paths = calloc (MAX_RESOURCES, sizeof (char *));

  if (!manager->resources || !manager->path_cache.ids
      || !manager->path_cache.paths)
    {
      resource_manager_destroy (manager);
      return NULL;
    }

  manager->next_id = 1;
  return manager;
}

void
resource_manager_destroy (resource_manager_t *manager)
{
  if (!manager)
    return;

  for (size_t i = 0; i < manager->resource_count; i++)
    {
      resource_t *res = &manager->resources[i];
      free (res->data);
      free (res->file_path);
    }

  free (manager->resources);
  free (manager->path_cache.ids);
  free (manager->path_cache.paths);
  free (manager);
}

static time_t
get_file_mtime (const char *path)
{
  struct stat st;
  if (stat (path, &st) == 0)
    {
      return st.st_mtime;
    }
  return 0;
}

static void *
load_file (const char *path, size_t *out_size)
{
  FILE *file = fopen (path, "rb");
  if (!file)
    return NULL;

  fseek (file, 0, SEEK_END);
  size_t size = ftell (file);
  fseek (file, 0, SEEK_SET);

  void *data = malloc (size);
  if (!data)
    {
      fclose (file);
      return NULL;
    }

  fread (data, 1, size, file);
  fclose (file);

  if (out_size)
    *out_size = size;
  return data;
}

result_t
resource_load (resource_manager_t *manager, const char *path,
               resource_type_t type, resource_id_t *out_id)
{
  if (!manager || !path)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

  // Check if already loaded
  for (size_t i = 0; i < manager->path_cache.count; i++)
    {
      if (strcmp (manager->path_cache.paths[i], path) == 0)
        {
          resource_id_t id = manager->path_cache.ids[i];
          resource_acquire (manager, id);
          if (out_id)
            *out_id = id;
          return RESULT_SUCCESS;
        }
    }

  // Load file
  size_t size;
  void *data = load_file (path, &size);
  if (!data)
    {
      return RESULT_ERROR (RESULT_ERROR_FILE_NOT_FOUND,
                           "Failed to load resource");
    }

  // Create resource
  if (manager->resource_count >= manager->resource_capacity)
    {
      free (data);
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION, "Resource limit reached");
    }

  resource_id_t id = manager->next_id++;
  resource_t *res = &manager->resources[manager->resource_count++];

  res->id = id;
  res->type = type;
  res->data = data;
  res->size = size;
  res->file_path = strdup (path);
  res->last_modified = get_file_mtime (path);
  res->ref_count = 1;

  // Add to cache
  size_t cache_idx = manager->path_cache.count++;
  manager->path_cache.ids[cache_idx] = id;
  manager->path_cache.paths[cache_idx] = res->file_path;

  if (out_id)
    *out_id = id;
  return RESULT_SUCCESS;
}

resource_t *
resource_get (resource_manager_t *manager, resource_id_t id)
{
  if (!manager)
    return NULL;

  for (size_t i = 0; i < manager->resource_count; i++)
    {
      if (manager->resources[i].id == id)
        {
          return &manager->resources[i];
        }
    }

  return NULL;
}

resource_t *
resource_get_by_path (resource_manager_t *manager, const char *path)
{
  if (!manager || !path)
    return NULL;

  for (size_t i = 0; i < manager->path_cache.count; i++)
    {
      if (strcmp (manager->path_cache.paths[i], path) == 0)
        {
          return resource_get (manager, manager->path_cache.ids[i]);
        }
    }

  return NULL;
}

void
resource_acquire (resource_manager_t *manager, resource_id_t id)
{
  resource_t *res = resource_get (manager, id);
  if (res)
    {
      res->ref_count++;
    }
}

void
resource_release (resource_manager_t *manager, resource_id_t id)
{
  resource_t *res = resource_get (manager, id);
  if (res && res->ref_count > 0)
    {
      res->ref_count--;
    }
}

result_t
resource_check_reload (resource_manager_t *manager)
{
  if (!manager)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid manager");

  for (size_t i = 0; i < manager->resource_count; i++)
    {
      resource_t *res = &manager->resources[i];

      if (!res->file_path)
        continue;

      time_t current_mtime = get_file_mtime (res->file_path);
      if (current_mtime > res->last_modified)
        {
          printf ("[Resources] Hot reload: %s\n", res->file_path);
          resource_reload (manager, res->id);
        }
    }

  return RESULT_SUCCESS;
}

result_t
resource_reload (resource_manager_t *manager, resource_id_t id)
{
  resource_t *res = resource_get (manager, id);
  if (!res || !res->file_path)
    {
      return RESULT_ERROR (RESULT_ERROR_NOT_FOUND, "Resource not found");
    }

  size_t new_size;
  void *new_data = load_file (res->file_path, &new_size);
  if (!new_data)
    {
      return RESULT_ERROR (RESULT_ERROR_FILE_NOT_FOUND,
                           "Failed to reload resource");
    }

  free (res->data);
  res->data = new_data;
  res->size = new_size;
  res->last_modified = get_file_mtime (res->file_path);

  return RESULT_SUCCESS;
}
