#include "prefab.h"
#include "component_parsers.h"
#include "scheme_parser.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Global Scheme state for prefab loading
static scheme_state_t *g_scheme_state = NULL;

// Prefab system lifecycle
prefab_system_t *
prefab_system_create (void)
{
  prefab_system_t *system = calloc (1, sizeof (prefab_system_t));
  if (!system)
    return NULL;

  system->prefab_capacity = 32;
  system->prefabs = calloc (system->prefab_capacity, sizeof (prefab_t));
  if (!system->prefabs)
    {
      free (system);
      return NULL;
    }

  // Initialize S7 Scheme
  g_scheme_state = scheme_init ();
  if (!g_scheme_state)
    {
      printf ("[Prefab] Warning: Failed to initialize S7 Scheme\n");
    }

  return system;
}

void
prefab_system_destroy (prefab_system_t *system)
{
  if (!system)
    return;

  for (size_t i = 0; i < system->prefab_count; i++)
    {
      prefab_cleanup (&system->prefabs[i]);
    }

  free (system->prefabs);
  free (system);

  // Shutdown S7 Scheme
  if (g_scheme_state)
    {
      scheme_shutdown (g_scheme_state);
      g_scheme_state = NULL;
    }
}

// Create empty prefab
prefab_t *
prefab_create (const char *name)
{
  prefab_t *prefab = calloc (1, sizeof (prefab_t));
  if (!prefab)
    return NULL;

  prefab->name = strdup (name);
  prefab->component_count = 0;
  return prefab;
}

// Add component to prefab
result_t
prefab_add_component (prefab_t *prefab, const char *component_name,
                      const void *data, size_t data_size)
{
  if (!prefab || !component_name || !data)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");
    }

  if (prefab->component_count >= MAX_PREFAB_COMPONENTS)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION, "Max components reached");
    }

  prefab_component_data_t *comp
      = &prefab->components[prefab->component_count];
  comp->component_name = strdup (component_name);
  comp->data_size = data_size;
  comp->data = malloc (data_size);
  if (!comp->data)
    {
      free ((void *)comp->component_name);
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION, "Failed to allocate data");
    }

  memcpy (comp->data, data, data_size);
  prefab->component_count++;

  return RESULT_SUCCESS;
}

// Cleanup prefab
void
prefab_cleanup (prefab_t *prefab)
{
  if (!prefab)
    return;

  if (prefab->name)
    free ((void *)prefab->name);
  if (prefab->description)
    free ((void *)prefab->description);

  for (size_t i = 0; i < prefab->component_count; i++)
    {
      if (prefab->components[i].component_name)
        free ((void *)prefab->components[i].component_name);
      if (prefab->components[i].data)
        free (prefab->components[i].data);
    }

  if (prefab->child_prefab_paths)
    {
      for (size_t i = 0; i < prefab->child_count; i++)
        {
          if (prefab->child_prefab_paths[i])
            free ((void *)prefab->child_prefab_paths[i]);
        }
      free ((void *)prefab->child_prefab_paths);
    }
}

// Load prefab from Scheme file using S7
result_t
prefab_load (prefab_system_t *system, const char *filepath,
             prefab_t **out_prefab)
{
  if (!system || !filepath)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");
    }

  if (!g_scheme_state)
    {
      return RESULT_ERROR (RESULT_ERROR_DEPENDENCY_MISSING,
                           "S7 Scheme not initialized");
    }

  // Load and evaluate Scheme file
  s7_pointer result;
  result_t load_result = scheme_load_file (g_scheme_state, filepath, &result);
  if (load_result.code != RESULT_OK)
    return load_result;

  // Verify it's a prefab definition: (prefab ...)
  if (!s7_is_pair_wrapper (g_scheme_state, result))
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Expected prefab definition");
    }

  s7_pointer tag = s7_car_wrapper (g_scheme_state, result);
  if (!s7_is_symbol_wrapper (g_scheme_state, tag))
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Expected symbol 'prefab'");
    }

  const char *tag_name = s7_symbol_name_wrapper (g_scheme_state, tag);
  if (!tag_name || strcmp (tag_name, "prefab") != 0)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Expected (prefab ...)");
    }

  // Expand capacity if needed
  if (system->prefab_count >= system->prefab_capacity)
    {
      size_t new_capacity = system->prefab_capacity * 2;
      prefab_t *new_prefabs
          = realloc (system->prefabs, new_capacity * sizeof (prefab_t));
      if (!new_prefabs)
        {
          return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                               "Failed to expand prefab storage");
        }
      system->prefabs = new_prefabs;
      system->prefab_capacity = new_capacity;
    }

  prefab_t *prefab = &system->prefabs[system->prefab_count];
  memset (prefab, 0, sizeof (prefab_t));

  // Parse fields: (name "..."), (description "..."), (component ...)
  s7_pointer current = s7_cdr_wrapper (g_scheme_state, result);

  while (s7_is_pair_wrapper (g_scheme_state, current))
    {
      s7_pointer field = s7_car_wrapper (g_scheme_state, current);

      if (s7_is_pair_wrapper (g_scheme_state, field))
        {
          s7_pointer field_name_obj = s7_car_wrapper (g_scheme_state, field);
          if (!s7_is_symbol_wrapper (g_scheme_state, field_name_obj))
            {
              current = s7_cdr_wrapper (g_scheme_state, current);
              continue;
            }

          const char *field_name
              = s7_symbol_name_wrapper (g_scheme_state, field_name_obj);
          if (!field_name)
            {
              current = s7_cdr_wrapper (g_scheme_state, current);
              continue;
            }

          // Parse name
          if (strcmp (field_name, "name") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (g_scheme_state, field);
              if (s7_is_string_wrapper (g_scheme_state, value))
                {
                  const char *name = s7_string_wrapper (g_scheme_state, value);
                  if (name)
                    prefab->name = strdup (name);
                }
            }
          // Parse description
          else if (strcmp (field_name, "description") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (g_scheme_state, field);
              if (s7_is_string_wrapper (g_scheme_state, value))
                {
                  const char *desc = s7_string_wrapper (g_scheme_state, value);
                  if (desc)
                    prefab->description = strdup (desc);
                }
            }
          // Parse component
          else if (strcmp (field_name, "component") == 0)
            {
              // Get component name
              s7_pointer comp_name_obj = s7_cadr_wrapper (g_scheme_state, field);
              if (!s7_is_string_wrapper (g_scheme_state, comp_name_obj))
                {
                  printf ("[Prefab] Warning: Component name must be a string\n");
                  current = s7_cdr_wrapper (g_scheme_state, current);
                  continue;
                }

              const char *comp_name
                  = s7_string_wrapper (g_scheme_state, comp_name_obj);
              printf ("[Prefab] Parsing component: %s\n", comp_name);

              // Parse component data based on type
              if (strcmp (comp_name, "shape") == 0)
                {
                  shape_component_t shape_data;
                  result_t res = parse_shape_component (g_scheme_state, field,
                                                        &shape_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (prefab, "shape", &shape_data,
                                            sizeof (shape_component_t));
                      printf ("[Prefab]   ? Shape component parsed\n");
                    }
                  else
                    {
                      printf ("[Prefab]   ? Failed to parse shape: %s\n",
                              res.message);
                    }
                }
              else if (strcmp (comp_name, "camera") == 0)
                {
                  camera_component_t camera_data;
                  result_t res = parse_camera_component (g_scheme_state, field,
                                                         &camera_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (prefab, "camera", &camera_data,
                                            sizeof (camera_component_t));
                      printf ("[Prefab]   ? Camera component parsed\n");
                    }
                  else
                    {
                      printf ("[Prefab]   ? Failed to parse camera: %s\n",
                              res.message);
                    }
                }
              else if (strcmp (comp_name, "camera_movement") == 0)
                {
                  camera_movement_component_t movement_data;
                  result_t res
                      = parse_camera_movement_component (g_scheme_state, field,
                                                         &movement_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (
                          prefab, "camera_movement", &movement_data,
                          sizeof (camera_movement_component_t));
                      printf ("[Prefab]   ? Camera movement component "
                              "parsed\n");
                    }
                  else
                    {
                      printf (
                          "[Prefab]   ? Failed to parse camera_movement: %s\n",
                          res.message);
                    }
                }
              else if (strcmp (comp_name, "camera_rotation") == 0)
                {
                  camera_rotation_component_t rotation_data;
                  result_t res
                      = parse_camera_rotation_component (g_scheme_state, field,
                                                         &rotation_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (
                          prefab, "camera_rotation", &rotation_data,
                          sizeof (camera_rotation_component_t));
                      printf ("[Prefab]   ? Camera rotation component "
                              "parsed\n");
                    }
                  else
                    {
                      printf (
                          "[Prefab]   ? Failed to parse camera_rotation: %s\n",
                          res.message);
                    }
                }
              else
                {
                  printf ("[Prefab]   ! Unknown component type: %s\n",
                          comp_name);
                }
            }
        }

      current = s7_cdr_wrapper (g_scheme_state, current);
    }

  system->prefab_count++;

  if (out_prefab)
    *out_prefab = prefab;

  printf ("[Prefab] Loaded '%s' from %s (S7 Scheme, %zu components)\n",
          prefab->name ? prefab->name : "Unnamed", filepath,
          prefab->component_count);

  return RESULT_SUCCESS;
}

// Load all prefabs from directory
result_t
prefab_load_directory (prefab_system_t *system, const char *directory_path)
{
  if (!system || !directory_path)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");
    }

  DIR *dir = opendir (directory_path);
  if (!dir)
    {
      char err[256];
      snprintf (err, sizeof (err), "Failed to open directory: %s",
                directory_path);
      return RESULT_ERROR (RESULT_ERROR_FILE_NOT_FOUND, err);
    }

  struct dirent *entry;
  size_t loaded_count = 0;

  while ((entry = readdir (dir)) != NULL)
    {
      // Skip . and ..
      if (strcmp (entry->d_name, ".") == 0 || strcmp (entry->d_name, "..") == 0)
        continue;

      // Check if file ends with .scm
      size_t name_len = strlen (entry->d_name);
      if (name_len < 4 || strcmp (entry->d_name + name_len - 4, ".scm") != 0)
        continue;

      // Build full path
      char filepath[512];
      snprintf (filepath, sizeof (filepath), "%s/%s", directory_path,
                entry->d_name);

      // Load prefab
      result_t result = prefab_load (system, filepath, NULL);
      if (result.code == RESULT_OK)
        {
          loaded_count++;
        }
      else
        {
          printf ("[Prefab] Warning: Failed to load %s: %s\n", filepath,
                  result.message);
        }
    }

  closedir (dir);

  printf ("[Prefab] Loaded %zu prefabs from %s\n", loaded_count,
          directory_path);

  return RESULT_SUCCESS;
}

// Find prefab by name
prefab_t *
prefab_find (prefab_system_t *system, const char *name)
{
  if (!system || !name)
    return NULL;

  for (size_t i = 0; i < system->prefab_count; i++)
    {
      if (strcmp (system->prefabs[i].name, name) == 0)
        {
          return &system->prefabs[i];
        }
    }

  return NULL;
}

// Instantiate prefab into world
result_t
prefab_instantiate (const prefab_t *prefab, ecs_world_t *world,
                    entity_id_t *out_entity)
{
  if (!prefab || !world)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");
    }

  entity_id_t entity = ecs_entity_create (world);
  if (entity == INVALID_ENTITY)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Failed to create entity");
    }

  // Add all components
  for (size_t i = 0; i < prefab->component_count; i++)
    {
      const prefab_component_data_t *comp = &prefab->components[i];

      component_id_t comp_id
          = ecs_get_component_id (world, comp->component_name);
      if (comp_id == INVALID_ENTITY)
        {
          printf ("[Prefab] Warning: Component '%s' not registered in world\n",
                  comp->component_name);
          continue;
        }

      result_t result = ecs_add_component (world, entity, comp_id, comp->data);
      if (result.code != RESULT_OK)
        {
          printf ("[Prefab] Warning: Failed to add component '%s': %s\n",
                  comp->component_name, result.message);
        }
      else
        {
          printf ("[Prefab]   ? Added component '%s' to entity %u\n",
                  comp->component_name, entity);
        }
    }

  if (out_entity)
    *out_entity = entity;

  printf ("[Prefab] Instantiated '%s' as entity %u (%zu components)\n",
          prefab->name, entity, prefab->component_count);

  return RESULT_SUCCESS;
}
