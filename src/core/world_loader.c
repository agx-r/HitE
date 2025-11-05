#include "world_loader.h"
#include "component_parsers.h"
#include "scheme_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global Scheme state for world loading
static scheme_state_t *g_world_scheme_state = NULL;

// Initialize world loader (call once at startup)
static void
world_loader_init (void)
{
  if (!g_world_scheme_state)
    {
      g_world_scheme_state = scheme_init ();
      if (!g_world_scheme_state)
        {
          printf ("[World Loader] Warning: Failed to initialize S7 Scheme\n");
        }
    }
}

// Parse component override or definition from Scheme
static result_t
parse_component_definition (scheme_state_t *state, s7_pointer comp_sexp,
                            const char **out_name, void **out_data,
                            size_t *out_size)
{
  if (!s7_is_pair_wrapper (state, comp_sexp))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Component must be a list");

  // Get component name: (component "name" ...)
  s7_pointer name_obj = s7_cadr_wrapper (state, comp_sexp);
  if (!s7_is_string_wrapper (state, name_obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Component name must be a string");

  const char *comp_name = s7_string_wrapper (state, name_obj);
  *out_name = strdup (comp_name);

  // Parse component data based on type
  if (strcmp (comp_name, "shape") == 0)
    {
      shape_component_t *shape_data = malloc (sizeof (shape_component_t));
      result_t res = parse_shape_component (state, comp_sexp, shape_data);
      if (res.code != RESULT_OK)
        {
          free (shape_data);
          return res;
        }
      *out_data = shape_data;
      *out_size = sizeof (shape_component_t);
    }
  else if (strcmp (comp_name, "camera") == 0)
    {
      camera_component_t *camera_data = malloc (sizeof (camera_component_t));
      result_t res = parse_camera_component (state, comp_sexp, camera_data);
      if (res.code != RESULT_OK)
        {
          free (camera_data);
          return res;
        }
      *out_data = camera_data;
      *out_size = sizeof (camera_component_t);
    }
  else if (strcmp (comp_name, "camera_movement") == 0)
    {
      camera_movement_component_t *movement_data
          = malloc (sizeof (camera_movement_component_t));
      result_t res
          = parse_camera_movement_component (state, comp_sexp, movement_data);
      if (res.code != RESULT_OK)
        {
          free (movement_data);
          return res;
        }
      *out_data = movement_data;
      *out_size = sizeof (camera_movement_component_t);
    }
  else if (strcmp (comp_name, "camera_rotation") == 0)
    {
      camera_rotation_component_t *rotation_data
          = malloc (sizeof (camera_rotation_component_t));
      result_t res
          = parse_camera_rotation_component (state, comp_sexp, rotation_data);
      if (res.code != RESULT_OK)
        {
          free (rotation_data);
          return res;
        }
      *out_data = rotation_data;
      *out_size = sizeof (camera_rotation_component_t);
    }
  else
    {
      char err[128];
      snprintf (err, sizeof (err), "Unknown component type: %s", comp_name);
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, err);
    }

  return RESULT_SUCCESS;
}

// Parse entity template from (entity "name" ...)
static result_t
parse_entity_template (scheme_state_t *state, s7_pointer entity_sexp,
                       entity_template_t *out_template)
{
  memset (out_template, 0, sizeof (entity_template_t));

  if (!s7_is_pair_wrapper (state, entity_sexp))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Entity must be a list");

  // Get entity name: (entity "name" ...)
  s7_pointer name_obj = s7_cadr_wrapper (state, entity_sexp);
  if (!s7_is_string_wrapper (state, name_obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Entity name must be a string");

  out_template->name = strdup (s7_string_wrapper (state, name_obj));

  // Count components
  size_t component_count = 0;
  s7_pointer current = s7_cdr_wrapper (state, entity_sexp);
  current = s7_cdr_wrapper (state, current); // Skip name

  while (s7_is_pair_wrapper (state, current))
    {
      s7_pointer field = s7_car_wrapper (state, current);
      if (s7_is_pair_wrapper (state, field))
        {
          s7_pointer field_name_obj = s7_car_wrapper (state, field);
          if (s7_is_symbol_wrapper (state, field_name_obj))
            {
              const char *field_name
                  = s7_symbol_name_wrapper (state, field_name_obj);
              if (strcmp (field_name, "component") == 0)
                component_count++;
            }
        }
      current = s7_cdr_wrapper (state, current);
    }

  // Allocate components
  if (component_count > 0)
    {
      out_template->components = calloc (
          component_count,
          sizeof (*out_template->components));
      out_template->component_count = 0;
    }

  // Parse components
  current = s7_cdr_wrapper (state, entity_sexp);
  current = s7_cdr_wrapper (state, current); // Skip name

  while (s7_is_pair_wrapper (state, current))
    {
      s7_pointer field = s7_car_wrapper (state, current);
      if (s7_is_pair_wrapper (state, field))
        {
          s7_pointer field_name_obj = s7_car_wrapper (state, field);
          if (s7_is_symbol_wrapper (state, field_name_obj))
            {
              const char *field_name
                  = s7_symbol_name_wrapper (state, field_name_obj);
              if (strcmp (field_name, "component") == 0)
                {
                  const char *comp_name;
                  void *comp_data;
                  size_t comp_size;

                  result_t res = parse_component_definition (
                      state, field, &comp_name, &comp_data, &comp_size);
                  if (res.code == RESULT_OK)
                    {
                      size_t idx = out_template->component_count++;
                      out_template->components[idx].component_name = comp_name;
                      out_template->components[idx].data = comp_data;
                      out_template->components[idx].data_size = comp_size;
                    }
                  else
                    {
                      printf ("[World Loader] Warning: Failed to parse "
                              "component: %s\n",
                              res.message);
                    }
                }
            }
        }
      current = s7_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

// Load world definition from Scheme file
result_t
world_load_from_file (const char *filepath, world_definition_t *out_definition)
{
  if (!filepath || !out_definition)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  world_loader_init ();

  if (!g_world_scheme_state)
    return RESULT_ERROR (RESULT_ERROR_DEPENDENCY_MISSING,
                         "S7 Scheme not initialized");

  memset (out_definition, 0, sizeof (world_definition_t));

  // Load and evaluate Scheme file
  s7_pointer result;
  result_t load_result
      = scheme_load_file (g_world_scheme_state, filepath, &result);
  if (load_result.code != RESULT_OK)
    return load_result;

  // Verify it's a world definition: (world ...)
  if (!s7_is_pair_wrapper (g_world_scheme_state, result))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected world definition");

  s7_pointer tag = s7_car_wrapper (g_world_scheme_state, result);
  if (!s7_is_symbol_wrapper (g_world_scheme_state, tag))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected symbol 'world'");

  const char *tag_name = s7_symbol_name_wrapper (g_world_scheme_state, tag);
  if (!tag_name || strcmp (tag_name, "world") != 0)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected (world ...)");

  // Parse world fields
  s7_pointer current = s7_cdr_wrapper (g_world_scheme_state, result);

  // Temporary arrays for counting
  size_t prefab_count = 0;
  size_t entity_count = 0;

  // First pass: count items
  s7_pointer count_current = current;
  while (s7_is_pair_wrapper (g_world_scheme_state, count_current))
    {
      s7_pointer field = s7_car_wrapper (g_world_scheme_state, count_current);
      if (s7_is_pair_wrapper (g_world_scheme_state, field))
        {
          s7_pointer field_name_obj
              = s7_car_wrapper (g_world_scheme_state, field);
          if (s7_is_symbol_wrapper (g_world_scheme_state, field_name_obj))
            {
              const char *field_name
                  = s7_symbol_name_wrapper (g_world_scheme_state,
                                            field_name_obj);
              if (strcmp (field_name, "prefabs") == 0)
                {
                  // Count prefab names in list
                  s7_pointer prefab_list
                      = s7_cdr_wrapper (g_world_scheme_state, field);
                  while (s7_is_pair_wrapper (g_world_scheme_state,
                                             prefab_list))
                    {
                      prefab_count++;
                      prefab_list
                          = s7_cdr_wrapper (g_world_scheme_state, prefab_list);
                    }
                }
              else if (strcmp (field_name, "entity") == 0)
                {
                  entity_count++;
                }
            }
        }
      count_current = s7_cdr_wrapper (g_world_scheme_state, count_current);
    }

  // Allocate arrays
  if (prefab_count > 0)
    {
      out_definition->prefab_instances
          = calloc (prefab_count, sizeof (prefab_instance_t));
      out_definition->prefab_instance_count = 0;
    }

  if (entity_count > 0)
    {
      out_definition->entity_templates
          = calloc (entity_count, sizeof (entity_template_t));
      out_definition->entity_template_count = 0;
    }

  // Second pass: parse fields
  while (s7_is_pair_wrapper (g_world_scheme_state, current))
    {
      s7_pointer field = s7_car_wrapper (g_world_scheme_state, current);

      if (s7_is_pair_wrapper (g_world_scheme_state, field))
        {
          s7_pointer field_name_obj
              = s7_car_wrapper (g_world_scheme_state, field);
          if (!s7_is_symbol_wrapper (g_world_scheme_state, field_name_obj))
            {
              current = s7_cdr_wrapper (g_world_scheme_state, current);
              continue;
            }

          const char *field_name
              = s7_symbol_name_wrapper (g_world_scheme_state, field_name_obj);
          if (!field_name)
            {
              current = s7_cdr_wrapper (g_world_scheme_state, current);
              continue;
            }

          // Parse name
          if (strcmp (field_name, "name") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (g_world_scheme_state, field);
              if (s7_is_string_wrapper (g_world_scheme_state, value))
                {
                  const char *name
                      = s7_string_wrapper (g_world_scheme_state, value);
                  if (name)
                    out_definition->name = strdup (name);
                }
            }
          // Parse fixed-delta-time
          else if (strcmp (field_name, "fixed-delta-time") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (g_world_scheme_state, field);
              s7_parse_float (g_world_scheme_state, value,
                              &out_definition->fixed_delta_time);
            }
          // Parse use-fixed-timestep
          else if (strcmp (field_name, "use-fixed-timestep") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (g_world_scheme_state, field);
              if (s7_is_boolean_wrapper (g_world_scheme_state, value))
                out_definition->use_fixed_timestep
                    = s7_boolean_wrapper (g_world_scheme_state, value);
            }
          // Parse prefabs list
          else if (strcmp (field_name, "prefabs") == 0)
            {
              s7_pointer prefab_list
                  = s7_cdr_wrapper (g_world_scheme_state, field);
              while (s7_is_pair_wrapper (g_world_scheme_state, prefab_list))
                {
                  s7_pointer prefab_name_obj
                      = s7_car_wrapper (g_world_scheme_state, prefab_list);
                  if (s7_is_string_wrapper (g_world_scheme_state,
                                            prefab_name_obj))
                    {
                      const char *prefab_name = s7_string_wrapper (
                          g_world_scheme_state, prefab_name_obj);
                      size_t idx = out_definition->prefab_instance_count++;
                      out_definition->prefab_instances[idx].prefab_name
                          = strdup (prefab_name);
                      printf ("[World Loader] Found prefab reference: %s\n",
                              prefab_name);
                    }
                  prefab_list
                      = s7_cdr_wrapper (g_world_scheme_state, prefab_list);
                }
            }
          // Parse entity
          else if (strcmp (field_name, "entity") == 0)
            {
              size_t idx = out_definition->entity_template_count;
              result_t res = parse_entity_template (
                  g_world_scheme_state, field,
                  &out_definition->entity_templates[idx]);
              if (res.code == RESULT_OK)
                {
                  out_definition->entity_template_count++;
                  printf ("[World Loader] Parsed entity template: %s (%zu "
                          "components)\n",
                          out_definition->entity_templates[idx].name,
                          out_definition->entity_templates[idx].component_count);
                }
              else
                {
                  printf ("[World Loader] Warning: Failed to parse entity: "
                          "%s\n",
                          res.message);
                }
            }
        }

      current = s7_cdr_wrapper (g_world_scheme_state, current);
    }

  printf ("[World Loader] Loaded world '%s' from %s\n",
          out_definition->name ? out_definition->name : "Unnamed", filepath);
  printf ("  Prefab instances: %zu\n", out_definition->prefab_instance_count);
  printf ("  Entity templates: %zu\n", out_definition->entity_template_count);

  return RESULT_SUCCESS;
}

// Free world definition
void
world_definition_free (world_definition_t *definition)
{
  if (!definition)
    return;

  if (definition->name)
    free ((void *)definition->name);

  // Free prefab instances
  if (definition->prefab_instances)
    {
      for (size_t i = 0; i < definition->prefab_instance_count; i++)
        {
          prefab_instance_t *inst = &definition->prefab_instances[i];
          if (inst->prefab_name)
            free ((void *)inst->prefab_name);
          if (inst->instance_name)
            free ((void *)inst->instance_name);

          // Free overrides
          if (inst->overrides)
            {
              for (size_t j = 0; j < inst->override_count; j++)
                {
                  if (inst->overrides[j].component_name)
                    free ((void *)inst->overrides[j].component_name);
                  if (inst->overrides[j].override_data)
                    free (inst->overrides[j].override_data);
                }
              free (inst->overrides);
            }

          // Free additional components
          if (inst->additional_components)
            {
              for (size_t j = 0; j < inst->additional_component_count; j++)
                {
                  if (inst->additional_components[j].component_name)
                    free ((void *)inst->additional_components[j].component_name);
                  if (inst->additional_components[j].override_data)
                    free (inst->additional_components[j].override_data);
                }
              free (inst->additional_components);
            }
        }
      free (definition->prefab_instances);
    }

  // Free entity templates
  if (definition->entity_templates)
    {
      for (size_t i = 0; i < definition->entity_template_count; i++)
        {
          entity_template_t *tmpl = &definition->entity_templates[i];
          if (tmpl->name)
            free ((void *)tmpl->name);

          if (tmpl->components)
            {
              for (size_t j = 0; j < tmpl->component_count; j++)
                {
                  if (tmpl->components[j].component_name)
                    free ((void *)tmpl->components[j].component_name);
                  if (tmpl->components[j].data)
                    free (tmpl->components[j].data);
                }
              free (tmpl->components);
            }
        }
      free (definition->entity_templates);
    }
}
