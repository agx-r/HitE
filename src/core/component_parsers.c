#include "component_parsers.h"
#include <stdio.h>
#include <string.h>

// Helper: Parse shape type from symbol (e.g., 'box, 'sphere, 'torus)
result_t
parse_shape_type (scheme_state_t *state, s7_pointer sexp,
                  shape_type_t *out_type)
{
  if (!state || !sexp || !out_type)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  // Type can be either a symbol or a string
  const char *type_str = NULL;
  
  if (s7_is_string_wrapper (state, sexp))
    {
      type_str = s7_string_wrapper (state, sexp);
      printf ("[Component Parser] Type is a string: %s\n", type_str);
    }
  else if (s7_is_symbol_wrapper (state, sexp))
    {
      type_str = s7_symbol_name_wrapper (state, sexp);
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

  // Match shape type
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
parse_shape_component (scheme_state_t *state, s7_pointer sexp,
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
  if (!s7_is_pair_wrapper (state, sexp))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Invalid component format");

  // Iterate through fields (skip first two: 'component' and "shape")
  s7_pointer current = s7_cdr_wrapper (state, sexp);
  if (s7_is_pair_wrapper (state, current))
    current = s7_cdr_wrapper (state, current); // Skip component name

  while (s7_is_pair_wrapper (state, current))
    {
      s7_pointer field = s7_car_wrapper (state, current);

      if (s7_is_pair_wrapper (state, field))
        {
          s7_pointer field_name_obj = s7_car_wrapper (state, field);
          if (!s7_is_symbol_wrapper (state, field_name_obj))
            {
              current = s7_cdr_wrapper (state, current);
              continue;
            }

          const char *field_name
              = s7_symbol_name_wrapper (state, field_name_obj);
          if (!field_name)
            {
              current = s7_cdr_wrapper (state, current);
              continue;
            }

          // Parse type
          if (strcmp (field_name, "type") == 0)
            {
              // field is (type "torus"), get the value after 'type'
              s7_pointer type_value = s7_car_wrapper (state, s7_cdr_wrapper (state, field));
              result_t res
                  = parse_shape_type (state, type_value, &out_component->type);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning: %s\n", res.message);
            }
          // Parse position (position x y z)
          else if (strcmp (field_name, "position") == 0)
            {
              s7_pointer pos_list = s7_cdr_wrapper (state, field);
              result_t res = s7_parse_vec3 (state, pos_list,
                                            &out_component->transform.position);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning parsing position: %s\n",
                        res.message);
            }
          // Parse dimensions
          else if (strcmp (field_name, "dimensions") == 0)
            {
              s7_pointer dim_list = s7_cdr_wrapper (state, field);
              result_t res
                  = s7_parse_vec3 (state, dim_list, &out_component->dimensions);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning parsing dimensions: %s\n",
                        res.message);
            }
          // Parse color
          else if (strcmp (field_name, "color") == 0)
            {
              s7_pointer color_list = s7_cdr_wrapper (state, field);
              result_t res
                  = s7_parse_vec4 (state, color_list, &out_component->color);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning parsing color: %s\n",
                        res.message);
            }
          // Parse visible
          else if (strcmp (field_name, "visible") == 0)
            {
              s7_pointer visible_value = s7_cadr_wrapper (state, field);
              if (s7_is_boolean_wrapper (state, visible_value))
                out_component->visible
                    = s7_boolean_wrapper (state, visible_value);
            }
        }

      current = s7_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

// Parse camera component from (component "camera" ...)
result_t
parse_camera_component (scheme_state_t *state, s7_pointer sexp,
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
  s7_pointer current = s7_cdr_wrapper (state, sexp);
  if (s7_is_pair_wrapper (state, current))
    current = s7_cdr_wrapper (state, current);

  while (s7_is_pair_wrapper (state, current))
    {
      s7_pointer field = s7_car_wrapper (state, current);

      if (s7_is_pair_wrapper (state, field))
        {
          s7_pointer field_name_obj = s7_car_wrapper (state, field);
          if (!s7_is_symbol_wrapper (state, field_name_obj))
            {
              current = s7_cdr_wrapper (state, current);
              continue;
            }

          const char *field_name
              = s7_symbol_name_wrapper (state, field_name_obj);
          if (!field_name)
            {
              current = s7_cdr_wrapper (state, current);
              continue;
            }

          // Parse position
          if (strcmp (field_name, "position") == 0)
            {
              s7_pointer pos_list = s7_cdr_wrapper (state, field);
              s7_parse_vec3 (state, pos_list, &out_component->position);
            }
          // Parse direction
          else if (strcmp (field_name, "direction") == 0)
            {
              s7_pointer dir_list = s7_cdr_wrapper (state, field);
              s7_parse_vec3 (state, dir_list, &out_component->direction);
            }
          // Parse up
          else if (strcmp (field_name, "up") == 0)
            {
              s7_pointer up_list = s7_cdr_wrapper (state, field);
              s7_parse_vec3 (state, up_list, &out_component->up);
            }
          // Parse fov
          else if (strcmp (field_name, "fov") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              s7_parse_float (state, value, &out_component->fov);
            }
          // Parse near-plane
          else if (strcmp (field_name, "near-plane") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              s7_parse_float (state, value, &out_component->near_plane);
            }
          // Parse far-plane
          else if (strcmp (field_name, "far-plane") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              s7_parse_float (state, value, &out_component->far_plane);
            }
          // Parse active
          else if (strcmp (field_name, "active") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              if (s7_is_boolean_wrapper (state, value))
                out_component->is_active = s7_boolean_wrapper (state, value);
            }
        }

      current = s7_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

// Parse camera_movement component
result_t
parse_camera_movement_component (scheme_state_t *state, s7_pointer sexp,
                                 camera_movement_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  // Initialize with defaults
  memset (out_component, 0, sizeof (camera_movement_component_t));
  out_component->move_speed = 5.0f;
  out_component->enabled = true;

  // Skip 'component' and "camera_movement"
  s7_pointer current = s7_cdr_wrapper (state, sexp);
  if (s7_is_pair_wrapper (state, current))
    current = s7_cdr_wrapper (state, current);

  while (s7_is_pair_wrapper (state, current))
    {
      s7_pointer field = s7_car_wrapper (state, current);

      if (s7_is_pair_wrapper (state, field))
        {
          s7_pointer field_name_obj = s7_car_wrapper (state, field);
          if (!s7_is_symbol_wrapper (state, field_name_obj))
            {
              current = s7_cdr_wrapper (state, current);
              continue;
            }

          const char *field_name
              = s7_symbol_name_wrapper (state, field_name_obj);
          if (!field_name)
            {
              current = s7_cdr_wrapper (state, current);
              continue;
            }

          // Parse move-speed
          if (strcmp (field_name, "move-speed") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              s7_parse_float (state, value, &out_component->move_speed);
            }
          // Parse enabled
          else if (strcmp (field_name, "enabled") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              if (s7_is_boolean_wrapper (state, value))
                out_component->enabled = s7_boolean_wrapper (state, value);
            }
        }

      current = s7_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

// Parse camera_rotation component
result_t
parse_camera_rotation_component (scheme_state_t *state, s7_pointer sexp,
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
  s7_pointer current = s7_cdr_wrapper (state, sexp);
  if (s7_is_pair_wrapper (state, current))
    current = s7_cdr_wrapper (state, current);

  while (s7_is_pair_wrapper (state, current))
    {
      s7_pointer field = s7_car_wrapper (state, current);

      if (s7_is_pair_wrapper (state, field))
        {
          s7_pointer field_name_obj = s7_car_wrapper (state, field);
          if (!s7_is_symbol_wrapper (state, field_name_obj))
            {
              current = s7_cdr_wrapper (state, current);
              continue;
            }

          const char *field_name
              = s7_symbol_name_wrapper (state, field_name_obj);
          if (!field_name)
            {
              current = s7_cdr_wrapper (state, current);
              continue;
            }

          // Parse yaw
          if (strcmp (field_name, "yaw") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              s7_parse_float (state, value, &out_component->yaw);
            }
          // Parse pitch
          else if (strcmp (field_name, "pitch") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              s7_parse_float (state, value, &out_component->pitch);
            }
          // Parse look-sensitivity
          else if (strcmp (field_name, "look-sensitivity") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              s7_parse_float (state, value, &out_component->look_sensitivity);
            }
          // Parse max-pitch
          else if (strcmp (field_name, "max-pitch") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              s7_parse_float (state, value, &out_component->max_pitch);
            }
          // Parse min-pitch
          else if (strcmp (field_name, "min-pitch") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              s7_parse_float (state, value, &out_component->min_pitch);
            }
          // Parse mouse-captured
          else if (strcmp (field_name, "mouse-captured") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              if (s7_is_boolean_wrapper (state, value))
                out_component->mouse_captured
                    = s7_boolean_wrapper (state, value);
            }
          // Parse enabled
          else if (strcmp (field_name, "enabled") == 0)
            {
              s7_pointer value = s7_cadr_wrapper (state, field);
              if (s7_is_boolean_wrapper (state, value))
                out_component->enabled = s7_boolean_wrapper (state, value);
            }
        }

      current = s7_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}
