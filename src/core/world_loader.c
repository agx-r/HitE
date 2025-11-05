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
      g_world_scheme_state = hite_scheme_init ();
      if (!g_world_scheme_state)
        {
          printf ("[World Loader] Warning: Failed to initialize TinyScheme\n");
        }
    }
}

// Parse component override or definition from Scheme
static result_t
parse_component_definition (scheme_state_t *state, pointer comp_sexp,
                            const char **out_name, void **out_data,
                            size_t *out_size)
{
  if (!scheme_is_pair_wrapper (state, comp_sexp))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Component must be a list");

  // Get component name: (component "name" ...)
  pointer name_obj = scheme_cadr_wrapper (state, comp_sexp);
  if (!scheme_is_string_wrapper (state, name_obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Component name must be a string");

  const char *comp_name = scheme_string_wrapper (state, name_obj);
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

// Parse entity template from (entity ...) or (entity (prefab "name") ...)
static result_t
parse_entity_template (scheme_state_t *state, pointer entity_sexp,
                       entity_template_t *out_template)
{
  memset (out_template, 0, sizeof (entity_template_t));

  if (!scheme_is_pair_wrapper (state, entity_sexp))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Entity must be a list");

  // Start from the first element after 'entity
  pointer current = scheme_cdr_wrapper (state, entity_sexp);
  
  // Check if first element is (prefab "name") or a component
  if (scheme_is_pair_wrapper (state, current))
    {
      pointer first_elem = scheme_car_wrapper (state, current);
      
      // Check if it's (prefab "name")
      if (scheme_is_pair_wrapper (state, first_elem))
        {
          pointer first_symbol = scheme_car_wrapper (state, first_elem);
          if (scheme_is_symbol_wrapper (state, first_symbol))
            {
              const char *symbol_name = scheme_symbol_name_wrapper (state, first_symbol);
              if (strcmp (symbol_name, "prefab") == 0)
                {
                  // It's a prefab reference: (entity (prefab "name") ...)
                  pointer prefab_name_obj = scheme_cadr_wrapper (state, first_elem);
                  if (scheme_is_string_wrapper (state, prefab_name_obj))
                    {
                      out_template->prefab_name = strdup (scheme_string_wrapper (state, prefab_name_obj));
                    }
                  // Skip the prefab element
                  current = scheme_cdr_wrapper (state, current);
                }
            }
        }
    }

  // Generate a name if not set
  if (!out_template->name && !out_template->prefab_name)
    {
      static int entity_counter = 0;
      char name_buf[32];
      snprintf (name_buf, sizeof (name_buf), "entity_%d", entity_counter++);
      out_template->name = strdup (name_buf);
    }

  // Count components
  size_t component_count = 0;
  pointer count_current = current;

  while (scheme_is_pair_wrapper (state, count_current))
    {
      pointer field = scheme_car_wrapper (state, count_current);
      if (scheme_is_pair_wrapper (state, field))
        {
          pointer field_name_obj = scheme_car_wrapper (state, field);
          if (scheme_is_symbol_wrapper (state, field_name_obj))
            {
              const char *field_name
                  = scheme_symbol_name_wrapper (state, field_name_obj);
              if (strcmp (field_name, "component") == 0)
                component_count++;
            }
        }
      count_current = scheme_cdr_wrapper (state, count_current);
    }

  // Allocate components
  if (component_count > 0)
    {
      out_template->components = calloc (
          component_count,
          sizeof (*out_template->components));
      out_template->component_count = 0;
    }

  // Parse components (current is already positioned after prefab if it existed)
  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);
      if (scheme_is_pair_wrapper (state, field))
        {
          pointer field_name_obj = scheme_car_wrapper (state, field);
          if (scheme_is_symbol_wrapper (state, field_name_obj))
            {
              const char *field_name
                  = scheme_symbol_name_wrapper (state, field_name_obj);
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
      current = scheme_cdr_wrapper (state, current);
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
                         "TinyScheme not initialized");

  memset (out_definition, 0, sizeof (world_definition_t));

  // Load and evaluate Scheme file
  pointer result;
  result_t load_result
      = hite_scheme_load_file (g_world_scheme_state, filepath, &result);
  if (load_result.code != RESULT_OK)
    return load_result;

  // Verify it's a world definition: (world ...)
  if (!scheme_is_pair_wrapper (g_world_scheme_state, result))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected world definition");

  pointer tag = scheme_car_wrapper (g_world_scheme_state, result);
  if (!scheme_is_symbol_wrapper (g_world_scheme_state, tag))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected symbol 'world'");

  const char *tag_name = scheme_symbol_name_wrapper (g_world_scheme_state, tag);
  if (!tag_name || strcmp (tag_name, "world") != 0)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected (world ...)");

  // Parse world fields
  pointer current = scheme_cdr_wrapper (g_world_scheme_state, result);

  // Temporary arrays for counting
  size_t prefab_count = 0;
  size_t entity_count = 0;

  // First pass: count items
  pointer count_current = current;
  while (scheme_is_pair_wrapper (g_world_scheme_state, count_current))
    {
      pointer field = scheme_car_wrapper (g_world_scheme_state, count_current);
      if (scheme_is_pair_wrapper (g_world_scheme_state, field))
        {
          pointer field_name_obj
              = scheme_car_wrapper (g_world_scheme_state, field);
          if (scheme_is_symbol_wrapper (g_world_scheme_state, field_name_obj))
            {
              const char *field_name
                  = scheme_symbol_name_wrapper (g_world_scheme_state,
                                            field_name_obj);
              if (strcmp (field_name, "prefabs") == 0)
                {
                  // Count prefab names in list
                  pointer prefab_list
                      = scheme_cdr_wrapper (g_world_scheme_state, field);
                  while (scheme_is_pair_wrapper (g_world_scheme_state,
                                             prefab_list))
                    {
                      prefab_count++;
                      prefab_list
                          = scheme_cdr_wrapper (g_world_scheme_state, prefab_list);
                    }
                }
              else if (strcmp (field_name, "entity") == 0)
                {
                  entity_count++;
                }
            }
        }
      count_current = scheme_cdr_wrapper (g_world_scheme_state, count_current);
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
  while (scheme_is_pair_wrapper (g_world_scheme_state, current))
    {
      pointer field = scheme_car_wrapper (g_world_scheme_state, current);

      if (scheme_is_pair_wrapper (g_world_scheme_state, field))
        {
          pointer field_name_obj
              = scheme_car_wrapper (g_world_scheme_state, field);
          if (!scheme_is_symbol_wrapper (g_world_scheme_state, field_name_obj))
            {
              current = scheme_cdr_wrapper (g_world_scheme_state, current);
              continue;
            }

          const char *field_name
              = scheme_symbol_name_wrapper (g_world_scheme_state, field_name_obj);
          if (!field_name)
            {
              current = scheme_cdr_wrapper (g_world_scheme_state, current);
              continue;
            }

          // Parse name
          if (strcmp (field_name, "name") == 0)
            {
              pointer value = scheme_cadr_wrapper (g_world_scheme_state, field);
              if (scheme_is_string_wrapper (g_world_scheme_state, value))
                {
                  const char *name
                      = scheme_string_wrapper (g_world_scheme_state, value);
                  if (name)
                    out_definition->name = strdup (name);
                }
            }
          // Parse fixed-delta-time
          else if (strcmp (field_name, "fixed-delta-time") == 0)
            {
              pointer value = scheme_cadr_wrapper (g_world_scheme_state, field);
              scheme_parse_float (g_world_scheme_state, value,
                              &out_definition->fixed_delta_time);
            }
          // Parse use-fixed-timestep
          else if (strcmp (field_name, "use-fixed-timestep") == 0)
            {
              pointer value = scheme_cadr_wrapper (g_world_scheme_state, field);
              if (scheme_is_boolean_wrapper (g_world_scheme_state, value))
                out_definition->use_fixed_timestep
                    = scheme_boolean_wrapper (g_world_scheme_state, value);
            }
          // Parse prefabs list
          else if (strcmp (field_name, "prefabs") == 0)
            {
              pointer prefab_list
                  = scheme_cdr_wrapper (g_world_scheme_state, field);
              while (scheme_is_pair_wrapper (g_world_scheme_state, prefab_list))
                {
                  pointer prefab_name_obj
                      = scheme_car_wrapper (g_world_scheme_state, prefab_list);
                  if (scheme_is_string_wrapper (g_world_scheme_state,
                                            prefab_name_obj))
                    {
                      const char *prefab_name = scheme_string_wrapper (
                          g_world_scheme_state, prefab_name_obj);
                      size_t idx = out_definition->prefab_instance_count++;
                      out_definition->prefab_instances[idx].prefab_name
                          = strdup (prefab_name);
                      printf ("[World Loader] Found prefab reference: %s\n",
                              prefab_name);
                    }
                  prefab_list
                      = scheme_cdr_wrapper (g_world_scheme_state, prefab_list);
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

      current = scheme_cdr_wrapper (g_world_scheme_state, current);
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
