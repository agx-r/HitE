#include "prefab.h"
#include "../components/developer_overlay_component.h"
#include "component_parsers.h"
#include "logger.h"
#include "scheme_parser.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static scheme_state_t *g_scheme_state = NULL;

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

  g_scheme_state = hite_scheme_init ();
  if (!g_scheme_state)
    {
      LOG_WARNING ("Prefab", "Failed to initialize TinyScheme");
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
  if (system->prefabs_directory)
    free ((void *)system->prefabs_directory);
  free (system);

  if (g_scheme_state)
    {
      hite_scheme_shutdown (g_scheme_state);
      g_scheme_state = NULL;
    }
}

result_t
prefab_add_component (prefab_t *prefab, const char *component_name,
                      const void *data, size_t data_size)
{
  if (!prefab || !component_name || !data)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid arguments");
    }

  if (prefab->component_count >= MAX_PREFAB_COMPONENTS)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION, "Max components reached");
    }

  prefab_component_data_t *comp = &prefab->components[prefab->component_count];
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

result_t
prefab_load (prefab_system_t *system, const char *filepath,
             prefab_t **out_prefab)
{
  if (!system || !filepath)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid arguments");
    }

  if (!g_scheme_state)
    {
      return RESULT_ERROR (RESULT_ERROR_DEPENDENCY_MISSING,
                           "TinyScheme not initialized");
    }

  pointer result;
  result_t load_result
      = hite_scheme_load_file (g_scheme_state, filepath, &result);
  if (load_result.code != RESULT_OK)
    return load_result;

  if (!scheme_is_pair_wrapper (g_scheme_state, result))
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Expected prefab definition");
    }

  pointer tag = scheme_car_wrapper (g_scheme_state, result);
  if (!scheme_is_symbol_wrapper (g_scheme_state, tag))
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Expected symbol 'prefab'");
    }

  const char *tag_name = scheme_symbol_name_wrapper (g_scheme_state, tag);
  if (!tag_name || strcmp (tag_name, "prefab") != 0)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Expected (prefab ...)");
    }

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

  pointer current = scheme_cdr_wrapper (g_scheme_state, result);

  while (scheme_is_pair_wrapper (g_scheme_state, current))
    {
      pointer field = scheme_car_wrapper (g_scheme_state, current);

      if (scheme_is_pair_wrapper (g_scheme_state, field))
        {
          pointer field_name_obj = scheme_car_wrapper (g_scheme_state, field);
          if (!scheme_is_symbol_wrapper (g_scheme_state, field_name_obj))
            {
              current = scheme_cdr_wrapper (g_scheme_state, current);
              continue;
            }

          const char *field_name
              = scheme_symbol_name_wrapper (g_scheme_state, field_name_obj);
          if (!field_name)
            {
              current = scheme_cdr_wrapper (g_scheme_state, current);
              continue;
            }

          if (strcmp (field_name, "name") == 0)
            {
              pointer value = scheme_cadr_wrapper (g_scheme_state, field);
              if (scheme_is_string_wrapper (g_scheme_state, value))
                {
                  const char *name
                      = scheme_string_wrapper (g_scheme_state, value);
                  if (name)
                    prefab->name = strdup (name);
                }
            }

          else if (strcmp (field_name, "description") == 0)
            {
              pointer value = scheme_cadr_wrapper (g_scheme_state, field);
              if (scheme_is_string_wrapper (g_scheme_state, value))
                {
                  const char *desc
                      = scheme_string_wrapper (g_scheme_state, value);
                  if (desc)
                    prefab->description = strdup (desc);
                }
            }

          else if (strcmp (field_name, "component") == 0)
            {

              pointer comp_name_obj
                  = scheme_cadr_wrapper (g_scheme_state, field);
              if (!scheme_is_string_wrapper (g_scheme_state, comp_name_obj))
                {
                  LOG_WARNING ("Prefab", "Component name must be a string");
                  current = scheme_cdr_wrapper (g_scheme_state, current);
                  continue;
                }

              const char *comp_name
                  = scheme_string_wrapper (g_scheme_state, comp_name_obj);
              LOG_DEBUG ("Prefab", "Parsing component: %s", comp_name);

              if (strcmp (comp_name, "shape") == 0)
                {
                  shape_component_t shape_data;
                  result_t res = parse_shape_component (g_scheme_state, field,
                                                        &shape_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (prefab, "shape", &shape_data,
                                            sizeof (shape_component_t));
                      LOG_DEBUG ("Prefab", "Shape component parsed");
                    }
                  else
                    {
                      LOG_WARNING ("Prefab", "Failed to parse shape: %s",
                                   res.message);
                    }
                }
              else if (strcmp (comp_name, "transform") == 0)
                {
                  transform_component_t transform_data;
                  result_t res = parse_transform_component (
                      g_scheme_state, field, &transform_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (prefab, "transform",
                                            &transform_data,
                                            sizeof (transform_component_t));
                      LOG_DEBUG ("Prefab", "Transform component parsed");
                    }
                  else
                    {
                      LOG_WARNING ("Prefab", "Failed to parse transform: %s",
                                   res.message);
                    }
                }
              else if (strcmp (comp_name, "transform") == 0)
                {
                  transform_component_t transform_data;
                  result_t res = parse_transform_component (
                      g_scheme_state, field, &transform_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (prefab, "transform",
                                            &transform_data,
                                            sizeof (transform_component_t));
                      LOG_DEBUG ("Prefab", "Transform component parsed");
                    }
                  else
                    {
                      LOG_WARNING ("Prefab", "Failed to parse transform: %s",
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
                      LOG_DEBUG ("Prefab", "Camera component parsed");
                    }
                  else
                    {
                      LOG_WARNING ("Prefab", "Failed to parse camera: %s",
                                   res.message);
                    }
                }
              else if (strcmp (comp_name, "camera_movement") == 0)
                {
                  camera_movement_component_t movement_data;
                  result_t res = parse_camera_movement_component (
                      g_scheme_state, field, &movement_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (
                          prefab, "camera_movement", &movement_data,
                          sizeof (camera_movement_component_t));
                      LOG_DEBUG ("Prefab",
                                 "  Camera movement component parsed");
                    }
                  else
                    {
                      LOG_WARNING ("Prefab",
                                   "Failed to parse camera_movement: %s",
                                   res.message);
                    }
                }
              else if (strcmp (comp_name, "camera_rotation") == 0)
                {
                  camera_rotation_component_t rotation_data;
                  result_t res = parse_camera_rotation_component (
                      g_scheme_state, field, &rotation_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (
                          prefab, "camera_rotation", &rotation_data,
                          sizeof (camera_rotation_component_t));
                      LOG_DEBUG ("Prefab",
                                 "  Camera rotation component parsed");
                    }
                  else
                    {
                      LOG_WARNING ("Prefab",
                                   "Failed to parse camera_rotation: %s",
                                   res.message);
                    }
                }
              else if (strcmp (comp_name, "developer_overlay") == 0)
                {
                  developer_overlay_component_t overlay_data;
                  result_t res = parse_developer_overlay_component (
                      g_scheme_state, field, &overlay_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (
                          prefab, "developer_overlay", &overlay_data,
                          sizeof (developer_overlay_component_t));
                      LOG_DEBUG ("Prefab",
                                 "Developer overlay component parsed");
                    }
                  else
                    {
                      LOG_WARNING ("Prefab",
                                   "Failed to parse developer_overlay: %s",
                                   res.message);
                    }
                }
              else if (strcmp (comp_name, "player") == 0)
                {
                  player_component_t player_data;
                  result_t res = parse_player_component (g_scheme_state, field,
                                                         &player_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (prefab, "player", &player_data,
                                            sizeof (player_component_t));
                      LOG_DEBUG ("Prefab", "Player component parsed");
                    }
                  else
                    {
                      LOG_WARNING ("Prefab", "Failed to parse player: %s",
                                   res.message);
                    }
                }
              else if (strcmp (comp_name, "player_collider") == 0)
                {
                  player_collider_component_t collider_data;
                  result_t res = parse_player_collider_component (
                      g_scheme_state, field, &collider_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (
                          prefab, "player_collider", &collider_data,
                          sizeof (player_collider_component_t));
                      LOG_DEBUG ("Prefab", "Player collider component parsed");
                    }
                  else
                    {
                      LOG_WARNING ("Prefab",
                                   "Failed to parse player_collider: %s",
                                   res.message);
                    }
                }
              else if (strcmp (comp_name, "player_movement_controls") == 0)
                {
                  player_movement_controls_component_t controls_data;
                  result_t res = parse_player_movement_controls_component (
                      g_scheme_state, field, &controls_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (
                          prefab, "player_movement_controls", &controls_data,
                          sizeof (player_movement_controls_component_t));
                      LOG_DEBUG ("Prefab",
                                 "Player movement controls component parsed");
                    }
                  else
                    {
                      LOG_WARNING (
                          "Prefab",
                          "Failed to parse player_movement_controls: %s",
                          res.message);
                    }
                }
              else if (strcmp (comp_name, "player_movement") == 0)
                {
                  player_movement_component_t movement_data = { 0 };
                  result_t res = parse_player_movement_component (
                      g_scheme_state, field, &movement_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (
                          prefab, "player_movement", &movement_data,
                          sizeof (player_movement_component_t));
                      LOG_DEBUG ("Prefab", "Player movement component parsed");
                    }
                  else
                    {
                      LOG_WARNING ("Prefab",
                                   "Failed to parse player_movement: %s",
                                   res.message);
                    }
                }
              else if (strcmp (comp_name, "lighting") == 0)
                {
                  lighting_component_t lighting_data;
                  result_t res = parse_lighting_component (
                      g_scheme_state, field, &lighting_data);
                  if (res.code == RESULT_OK)
                    {
                      prefab_add_component (prefab, "lighting", &lighting_data,
                                            sizeof (lighting_component_t));
                      LOG_DEBUG ("Prefab", "Lightning component parsed");
                    }
                  else
                    {
                      LOG_WARNING ("Prefab", "Failed to parse lighting: %s",
                                   res.message);
                    }
                }
              else
                {
                  LOG_WARNING ("Prefab", "Unknown component type: %s",
                               comp_name);
                }
            }
        }

      current = scheme_cdr_wrapper (g_scheme_state, current);
    }

  system->prefab_count++;

  if (out_prefab)
    *out_prefab = prefab;

  LOG_INFO ("Prefab", "Loaded '%s' from %s (TinyScheme, %zu components)",
            prefab->name ? prefab->name : "Unnamed", filepath,
            prefab->component_count);

  return RESULT_SUCCESS;
}

void
prefab_system_set_directory (prefab_system_t *system,
                             const char *directory_path)
{
  if (!system)
    return;

  if (system->prefabs_directory)
    free ((void *)system->prefabs_directory);

  if (directory_path)
    system->prefabs_directory = strdup (directory_path);
  else
    system->prefabs_directory = NULL;
}

prefab_t *
prefab_find (prefab_system_t *system, const char *name)
{
  if (!system || !name)
    return NULL;

  for (size_t i = 0; i < system->prefab_count; i++)
    {
      if (system->prefabs[i].name
          && strcmp (system->prefabs[i].name, name) == 0)
        {
          return &system->prefabs[i];
        }
    }

  if (system->prefabs_directory)
    {
      char filepath[512];
      snprintf (filepath, sizeof (filepath), "%s/%s.scm",
                system->prefabs_directory, name);

      result_t result = prefab_load (system, filepath, NULL);
      if (result.code == RESULT_OK)
        {
          for (size_t i = 0; i < system->prefab_count; i++)
            {
              if (system->prefabs[i].name
                  && strcmp (system->prefabs[i].name, name) == 0)
                {
                  return &system->prefabs[i];
                }
            }
        }
    }

  return NULL;
}

result_t
prefab_instantiate (const prefab_t *prefab, ecs_world_t *world,
                    entity_id_t *out_entity)
{
  if (!prefab || !world)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid arguments");
    }

  entity_id_t entity = ecs_entity_create (world);
  if (entity == INVALID_ENTITY)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION, "Failed to create entity");
    }

  for (size_t i = 0; i < prefab->component_count; i++)
    {
      const prefab_component_data_t *comp = &prefab->components[i];

      component_id_t comp_id
          = ecs_get_component_id (world, comp->component_name);
      if (comp_id == INVALID_ENTITY)
        {
          LOG_WARNING ("Prefab", "Component '%s' not registered in world\n",
                       comp->component_name);
          continue;
        }

      result_t result = ecs_add_component (world, entity, comp_id, comp->data);
      if (result.code != RESULT_OK)
        {
          LOG_WARNING ("Prefab", "Failed to add component '%s': %s\n",
                       comp->component_name, result.message);
        }
      else
        {
          LOG_INFO ("Prefab", "Added component '%s' to entity %u\n",
                    comp->component_name, entity);
        }
    }

  if (out_entity)
    *out_entity = entity;

  LOG_INFO ("Prefab", "Instantiated '%s' as entity %u (%zu components)\n",
            prefab->name, entity, prefab->component_count);

  return RESULT_SUCCESS;
}
