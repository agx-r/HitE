#include "component_parsers.h"
#include "../components/shape_component.h"
#include <stdio.h>
#include <string.h>

// Helper: Apply partial component override (only update specified fields)
static void
apply_shape_component_override (scheme_state_t *state, pointer sexp,
                                shape_component_t *target)
{
  if (!state || !sexp || !target)
    return;

  if (!scheme_is_pair_wrapper (state, sexp))
    return;

  // Iterate through fields (skip first two: 'component' and "shape")
  pointer current = scheme_cdr_wrapper (state, sexp);
  if (scheme_is_pair_wrapper (state, current))
    current = scheme_cdr_wrapper (state, current); // Skip component name

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);

      if (scheme_is_pair_wrapper (state, field))
        {
          pointer field_name_obj = scheme_car_wrapper (state, field);
          if (!scheme_is_symbol_wrapper (state, field_name_obj))
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          const char *field_name
              = scheme_symbol_name_wrapper (state, field_name_obj);
          if (!field_name)
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          // Override specific fields only
          if (strcmp (field_name, "type") == 0)
            {
              pointer type_value = scheme_car_wrapper (
                  state, scheme_cdr_wrapper (state, field));
              parse_shape_type (state, type_value, &target->type);
            }
          else if (strcmp (field_name, "position") == 0)
            {
              pointer pos_list = scheme_cdr_wrapper (state, field);
              scheme_parse_vec3 (state, pos_list, &target->transform.position);
            }
          else if (strcmp (field_name, "dimensions") == 0)
            {
              pointer dim_list = scheme_cdr_wrapper (state, field);
              scheme_parse_vec3 (state, dim_list, &target->dimensions);
            }
          else if (strcmp (field_name, "color") == 0)
            {
              pointer color_list = scheme_cdr_wrapper (state, field);
              scheme_parse_vec4 (state, color_list, &target->color);
            }
          else if (strcmp (field_name, "visible") == 0)
            {
              pointer visible_value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, visible_value))
                target->visible
                    = scheme_boolean_wrapper (state, visible_value);
            }
        }

      current = scheme_cdr_wrapper (state, current);
    }

  // Mark as dirty after override
  target->dirty = true;
}

// Helper: Parse shape type from symbol (e.g., 'box, 'sphere, 'torus)
result_t
parse_shape_type (scheme_state_t *state, pointer sexp, shape_type_t *out_type)
{
  if (!state || !sexp || !out_type)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  const char *type_str = NULL;
  if (scheme_is_string_wrapper (state, sexp))
    {
      type_str = scheme_string_wrapper (state, sexp);
      printf ("[Component Parser] Type is a string: %s\n", type_str);
    }
  else if (scheme_is_symbol_wrapper (state, sexp))
    {
      type_str = scheme_symbol_name_wrapper (state, sexp);
      printf ("[Component Parser] Type is a symbol: %s\n", type_str);
    }
  else
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Shape type must be a string or symbol");
    }
  if (!type_str)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Failed to get shape type");

  if (strcmp (type_str, "sphere") == 0)
    *out_type = SHAPE_SPHERE;
  else if (strcmp (type_str, "box") == 0)
    *out_type = SHAPE_BOX;
  else if (strcmp (type_str, "torus") == 0)
    *out_type = SHAPE_TORUS;
  else if (strcmp (type_str, "plane") == 0)
    *out_type = SHAPE_PLANE;
  else if (strcmp (type_str, "cylinder") == 0)
    *out_type = SHAPE_CYLINDER;
  else if (strcmp (type_str, "capsule") == 0)
    *out_type = SHAPE_CAPSULE;
  else if (strcmp (type_str, "cone") == 0)
    *out_type = SHAPE_CONE;
  else if (strcmp (type_str, "terrain") == 0)
    *out_type = SHAPE_TERRAIN;
  else
    {
      char err[128];
      snprintf (err, sizeof (err), "Unknown shape type: %s", type_str);
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, err);
    }

  return RESULT_SUCCESS;
}

// Parse shape component from (component "shape" ...)
result_t
parse_shape_component (scheme_state_t *state, pointer sexp,
                       shape_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  // Initialize with defaults
  memset (out_component, 0, sizeof (shape_component_t));
  out_component->type = SHAPE_SPHERE;
  out_component->operation = SHAPE_OP_UNION;
  out_component->color = (vec4_t){ 1.0f, 1.0f, 1.0f, 1.0f };
  out_component->dimensions = (vec3_t){ 1.0f, 1.0f, 1.0f, 0.0f };
  out_component->transform.position = (vec3_t){ 0.0f, 0.0f, 0.0f, 0.0f };
  out_component->transform.scale = (vec3_t){ 1.0f, 1.0f, 1.0f, 0.0f };
  out_component->transform.rotation
      = (vec4_t){ 0.0f, 0.0f, 0.0f, 1.0f }; // identity quaternion
  out_component->visible = true;
  out_component->dirty = true;

  // sexp should be (component "shape" ...)
  if (!scheme_is_pair_wrapper (state, sexp))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Invalid component format");

  // Iterate through fields (skip first two: 'component' and "shape")
  pointer current = scheme_cdr_wrapper (state, sexp);
  if (scheme_is_pair_wrapper (state, current))
    current = scheme_cdr_wrapper (state, current); // Skip component name

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);

      if (scheme_is_pair_wrapper (state, field))
        {
          pointer field_name_obj = scheme_car_wrapper (state, field);
          if (!scheme_is_symbol_wrapper (state, field_name_obj))
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          const char *field_name
              = scheme_symbol_name_wrapper (state, field_name_obj);
          if (!field_name)
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          // Parse type
          if (strcmp (field_name, "type") == 0)
            {
              // field is (type "torus"), get the value after 'type'
              pointer type_value = scheme_car_wrapper (
                  state, scheme_cdr_wrapper (state, field));
              result_t res
                  = parse_shape_type (state, type_value, &out_component->type);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning: %s\n", res.message);
            }
          // Parse position (position x y z)
          else if (strcmp (field_name, "position") == 0)
            {
              pointer pos_list = scheme_cdr_wrapper (state, field);
              result_t res = scheme_parse_vec3 (
                  state, pos_list, &out_component->transform.position);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning parsing position: %s\n",
                        res.message);
            }
          // Parse dimensions
          else if (strcmp (field_name, "dimensions") == 0)
            {
              pointer dim_list = scheme_cdr_wrapper (state, field);
              result_t res = scheme_parse_vec3 (state, dim_list,
                                                &out_component->dimensions);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning parsing dimensions: %s\n",
                        res.message);
            }
          // Parse color
          else if (strcmp (field_name, "color") == 0)
            {
              pointer color_list = scheme_cdr_wrapper (state, field);
              result_t res = scheme_parse_vec4 (state, color_list,
                                                &out_component->color);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning parsing color: %s\n",
                        res.message);
            }
          // Parse visible
          else if (strcmp (field_name, "visible") == 0)
            {
              pointer visible_value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, visible_value))
                out_component->visible
                    = scheme_boolean_wrapper (state, visible_value);
            }
        }

      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

// Parse camera component from (component "camera" ...)
result_t
parse_camera_component (scheme_state_t *state, pointer sexp,
                        camera_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  // Initialize with defaults
  memset (out_component, 0, sizeof (camera_component_t));
  out_component->position = (vec3_t){ 0.0f, 0.0f, 0.0f, 0.0f };
  out_component->direction = (vec3_t){ 0.0f, 0.0f, 1.0f, 0.0f };
  out_component->up = (vec3_t){ 0.0f, 1.0f, 0.0f, 0.0f };
  out_component->fov = 70.0f;
  out_component->near_plane = 0.1f;
  out_component->far_plane = 1000.0f;
  out_component->is_active = true;

  // Skip 'component' and "camera"
  pointer current = scheme_cdr_wrapper (state, sexp);
  if (scheme_is_pair_wrapper (state, current))
    current = scheme_cdr_wrapper (state, current);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);

      if (scheme_is_pair_wrapper (state, field))
        {
          pointer field_name_obj = scheme_car_wrapper (state, field);
          if (!scheme_is_symbol_wrapper (state, field_name_obj))
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          const char *field_name
              = scheme_symbol_name_wrapper (state, field_name_obj);
          if (!field_name)
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          // Parse position
          if (strcmp (field_name, "position") == 0)
            {
              pointer pos_list = scheme_cdr_wrapper (state, field);
              scheme_parse_vec3 (state, pos_list, &out_component->position);
            }
          // Parse direction
          else if (strcmp (field_name, "direction") == 0)
            {
              pointer dir_list = scheme_cdr_wrapper (state, field);
              scheme_parse_vec3 (state, dir_list, &out_component->direction);
            }
          // Parse up
          else if (strcmp (field_name, "up") == 0)
            {
              pointer up_list = scheme_cdr_wrapper (state, field);
              scheme_parse_vec3 (state, up_list, &out_component->up);
            }
          // Parse fov
          else if (strcmp (field_name, "fov") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->fov);
            }
          // Parse near-plane
          else if (strcmp (field_name, "near-plane") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->near_plane);
            }
          // Parse far-plane
          else if (strcmp (field_name, "far-plane") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->far_plane);
            }
          // Parse active
          else if (strcmp (field_name, "active") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, value))
                out_component->is_active
                    = scheme_boolean_wrapper (state, value);
            }
        }

      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

// Parse camera_movement component
result_t
parse_camera_movement_component (scheme_state_t *state, pointer sexp,
                                 camera_movement_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  // Initialize with defaults
  memset (out_component, 0, sizeof (camera_movement_component_t));
  out_component->move_speed = 5.0f;
  out_component->enabled = true;

  // Skip 'component' and "camera_movement"
  pointer current = scheme_cdr_wrapper (state, sexp);
  if (scheme_is_pair_wrapper (state, current))
    current = scheme_cdr_wrapper (state, current);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);

      if (scheme_is_pair_wrapper (state, field))
        {
          pointer field_name_obj = scheme_car_wrapper (state, field);
          if (!scheme_is_symbol_wrapper (state, field_name_obj))
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          const char *field_name
              = scheme_symbol_name_wrapper (state, field_name_obj);
          if (!field_name)
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          // Parse move-speed
          if (strcmp (field_name, "move-speed") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->move_speed);
            }
          // Parse enabled
          else if (strcmp (field_name, "enabled") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, value))
                out_component->enabled = scheme_boolean_wrapper (state, value);
            }
        }

      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

// Parse camera_rotation component
result_t
parse_camera_rotation_component (scheme_state_t *state, pointer sexp,
                                 camera_rotation_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  // Initialize with defaults
  memset (out_component, 0, sizeof (camera_rotation_component_t));
  out_component->yaw = 0.0f;
  out_component->pitch = 0.0f;
  out_component->look_sensitivity = 0.003f;
  out_component->max_pitch = 1.5f;
  out_component->min_pitch = -1.5f;
  out_component->mouse_captured = false;
  out_component->enabled = true;
  out_component->first_mouse = true;

  // Skip 'component' and "camera_rotation"
  pointer current = scheme_cdr_wrapper (state, sexp);
  if (scheme_is_pair_wrapper (state, current))
    current = scheme_cdr_wrapper (state, current);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);

      if (scheme_is_pair_wrapper (state, field))
        {
          pointer field_name_obj = scheme_car_wrapper (state, field);
          if (!scheme_is_symbol_wrapper (state, field_name_obj))
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          const char *field_name
              = scheme_symbol_name_wrapper (state, field_name_obj);
          if (!field_name)
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          // Parse yaw
          if (strcmp (field_name, "yaw") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->yaw);
            }
          // Parse pitch
          else if (strcmp (field_name, "pitch") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->pitch);
            }
          // Parse look-sensitivity
          else if (strcmp (field_name, "look-sensitivity") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value,
                                  &out_component->look_sensitivity);
            }
          // Parse max-pitch
          else if (strcmp (field_name, "max-pitch") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->max_pitch);
            }
          // Parse min-pitch
          else if (strcmp (field_name, "min-pitch") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->min_pitch);
            }
          // Parse mouse-captured
          else if (strcmp (field_name, "mouse-captured") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, value))
                out_component->mouse_captured
                    = scheme_boolean_wrapper (state, value);
            }
          // Parse enabled
          else if (strcmp (field_name, "enabled") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, value))
                out_component->enabled = scheme_boolean_wrapper (state, value);
            }
        }

      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

// Parse gui component from (component "gui" ...)
result_t
parse_gui_component (scheme_state_t *state, pointer sexp,
                     gui_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  // Initialize with defaults
  memset (out_component, 0, sizeof (gui_component_t));
  out_component->enabled = true;
  out_component->camera_entity = INVALID_ENTITY;

  // Skip 'component' and "gui"
  pointer current = scheme_cdr_wrapper (state, sexp);
  if (scheme_is_pair_wrapper (state, current))
    current = scheme_cdr_wrapper (state, current);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);

      if (scheme_is_pair_wrapper (state, field))
        {
          pointer field_name_obj = scheme_car_wrapper (state, field);
          if (!scheme_is_symbol_wrapper (state, field_name_obj))
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          const char *field_name
              = scheme_symbol_name_wrapper (state, field_name_obj);
          if (!field_name)
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          // Parse enabled
          if (strcmp (field_name, "enabled") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, value))
                out_component->enabled = scheme_boolean_wrapper (state, value);
            }
        }

      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

// Parse developer_overlay component from (component "developer_overlay" ...)
result_t
parse_developer_overlay_component (
    scheme_state_t *state, pointer sexp,
    developer_overlay_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  // Initialize with defaults
  memset (out_component, 0, sizeof (developer_overlay_component_t));
  out_component->enabled = true;
  out_component->gui_entity = INVALID_ENTITY;
  out_component->fps_update_interval = 0.5f;

  // Skip 'component' and "developer_overlay"
  pointer current = scheme_cdr_wrapper (state, sexp);
  if (scheme_is_pair_wrapper (state, current))
    current = scheme_cdr_wrapper (state, current);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);

      if (scheme_is_pair_wrapper (state, field))
        {
          pointer field_name_obj = scheme_car_wrapper (state, field);
          if (!scheme_is_symbol_wrapper (state, field_name_obj))
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          const char *field_name
              = scheme_symbol_name_wrapper (state, field_name_obj);
          if (!field_name)
            {
              current = scheme_cdr_wrapper (state, current);
              continue;
            }

          // Parse enabled
          if (strcmp (field_name, "enabled") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, value))
                out_component->enabled = scheme_boolean_wrapper (state, value);
            }
          // Parse fps-update-interval
          else if (strcmp (field_name, "fps-update-interval") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value,
                                  &out_component->fps_update_interval);
            }
        }

      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

// Public function to apply component override based on type
void
apply_component_override (scheme_state_t *state, const char *component_name,
                          pointer sexp, void *target_component)
{
  if (!state || !component_name || !sexp || !target_component)
    return;

  if (strcmp (component_name, "shape") == 0)
    {
      apply_shape_component_override (state, sexp,
                                      (shape_component_t *)target_component);
    }
  // Add other component types as needed
  // else if (strcmp (component_name, "camera") == 0) { ... }
}
