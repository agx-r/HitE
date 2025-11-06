#include "component_parsers.h"
#include "../components/lighting_component.h"
#include "../components/shape_component.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static void
apply_shape_component_override (scheme_state_t *state, pointer sexp,
                                shape_component_t *target)
{
  if (!state || !sexp || !target)
    {
      printf ("[Component Override] Invalid arguments for shape override\n");
      return;
    }

  if (!scheme_is_pair_wrapper (state, sexp))
    {
      printf ("[Component Override] S-expression is not a pair\n");
      return;
    }

  printf ("[Component Override] Parsing shape component override\n");

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

          if (strcmp (field_name, "type") == 0)
            {
              pointer type_value = scheme_car_wrapper (
                  state, scheme_cdr_wrapper (state, field));
              parse_shape_type (state, type_value, &target->type);
            }
          else if (strcmp (field_name, "position") == 0)
            {
              pointer pos_list = scheme_cdr_wrapper (state, field);
              result_t res = scheme_parse_vec3 (state, pos_list,
                                                &target->transform.position);
              if (res.code == RESULT_OK)
                {
                  printf (
                      "[Component Override] Updated position to: %.2f %.2f "
                      "%.2f\n",
                      target->transform.position.x,
                      target->transform.position.y,
                      target->transform.position.z);
                }
              else
                {
                  printf (
                      "[Component Override] Failed to parse position: %s\n",
                      res.message);
                }
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

          else if (strcmp (field_name, "seed") == 0)
            {
              pointer seed_value = scheme_cadr_wrapper (state, field);
              if (target->type == SHAPE_TERRAIN
                  || target->type == SHAPE_CITADEL)
                {
                  scheme_parse_float (state, seed_value, &target->roughness);
                }
            }
        }

      current = scheme_cdr_wrapper (state, current);
    }

  target->dirty = true;
}

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
  else if (strcmp (type_str, "citadel") == 0)
    *out_type = SHAPE_CITADEL;
  else
    {
      char err[128];
      snprintf (err, sizeof (err), "Unknown shape type: %s", type_str);
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, err);
    }

  return RESULT_SUCCESS;
}

result_t
parse_shape_component (scheme_state_t *state, pointer sexp,
                       shape_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  memset (out_component, 0, sizeof (shape_component_t));
  out_component->type = SHAPE_SPHERE;
  out_component->operation = SHAPE_OP_UNION;
  out_component->color = (vec4_t){ 1.0f, 1.0f, 1.0f, 1.0f };
  out_component->dimensions = (vec3_t){ 1.0f, 1.0f, 1.0f, 0.0f };
  out_component->transform.position = (vec3_t){ 0.0f, 0.0f, 0.0f, 0.0f };
  out_component->transform.scale = (vec3_t){ 1.0f, 1.0f, 1.0f, 0.0f };
  out_component->transform.rotation = (vec4_t){ 0.0f, 0.0f, 0.0f, 1.0f };
  out_component->visible = true;
  out_component->dirty = true;

  if (!scheme_is_pair_wrapper (state, sexp))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Invalid component format");

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

          if (strcmp (field_name, "type") == 0)
            {

              pointer type_value = scheme_car_wrapper (
                  state, scheme_cdr_wrapper (state, field));
              result_t res
                  = parse_shape_type (state, type_value, &out_component->type);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning: %s\n", res.message);
            }

          else if (strcmp (field_name, "position") == 0)
            {
              pointer pos_list = scheme_cdr_wrapper (state, field);
              result_t res = scheme_parse_vec3 (
                  state, pos_list, &out_component->transform.position);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning parsing position: %s\n",
                        res.message);
            }

          else if (strcmp (field_name, "dimensions") == 0)
            {
              pointer dim_list = scheme_cdr_wrapper (state, field);
              result_t res = scheme_parse_vec3 (state, dim_list,
                                                &out_component->dimensions);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning parsing dimensions: %s\n",
                        res.message);
            }

          else if (strcmp (field_name, "color") == 0)
            {
              pointer color_list = scheme_cdr_wrapper (state, field);
              result_t res = scheme_parse_vec4 (state, color_list,
                                                &out_component->color);
              if (res.code != RESULT_OK)
                printf ("[Component Parser] Warning parsing color: %s\n",
                        res.message);
            }

          else if (strcmp (field_name, "visible") == 0)
            {
              pointer visible_value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, visible_value))
                out_component->visible
                    = scheme_boolean_wrapper (state, visible_value);
            }

          else if (strcmp (field_name, "seed") == 0)
            {
              pointer seed_value = scheme_cadr_wrapper (state, field);
              if (out_component->type == SHAPE_TERRAIN
                  || out_component->type == SHAPE_CITADEL)
                {
                  result_t res = scheme_parse_float (
                      state, seed_value, &out_component->roughness);
                  if (res.code != RESULT_OK)
                    printf ("[Component Parser] Warning parsing seed: %s\n",
                            res.message);
                }
            }
        }

      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

result_t
parse_camera_component (scheme_state_t *state, pointer sexp,
                        camera_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  memset (out_component, 0, sizeof (camera_component_t));
  out_component->position = (vec3_t){ 0.0f, 0.0f, 0.0f, 0.0f };
  out_component->direction = (vec3_t){ 0.0f, 0.0f, 1.0f, 0.0f };
  out_component->up = (vec3_t){ 0.0f, 1.0f, 0.0f, 0.0f };
  out_component->fov = 70.0f;
  out_component->near_plane = 0.1f;
  out_component->far_plane = 1000.0f;
  out_component->background_color = (vec3_t){ 0.06f, 0.06f, 0.06f, 0.0f };
  out_component->is_active = true;

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

          if (strcmp (field_name, "position") == 0)
            {
              pointer pos_list = scheme_cdr_wrapper (state, field);
              scheme_parse_vec3 (state, pos_list, &out_component->position);
            }

          else if (strcmp (field_name, "direction") == 0)
            {
              pointer dir_list = scheme_cdr_wrapper (state, field);
              scheme_parse_vec3 (state, dir_list, &out_component->direction);
            }

          else if (strcmp (field_name, "up") == 0)
            {
              pointer up_list = scheme_cdr_wrapper (state, field);
              scheme_parse_vec3 (state, up_list, &out_component->up);
            }

          else if (strcmp (field_name, "fov") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->fov);
            }

          else if (strcmp (field_name, "near-plane") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->near_plane);
            }

          else if (strcmp (field_name, "far-plane") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->far_plane);
            }

          else if (strcmp (field_name, "active") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, value))
                out_component->is_active
                    = scheme_boolean_wrapper (state, value);
            }

          else if (strcmp (field_name, "background-color") == 0)
            {
              pointer color_list = scheme_cdr_wrapper (state, field);
              scheme_parse_vec3 (state, color_list,
                                 &out_component->background_color);
            }
        }

      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

result_t
parse_camera_movement_component (scheme_state_t *state, pointer sexp,
                                 camera_movement_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  memset (out_component, 0, sizeof (camera_movement_component_t));
  out_component->move_speed = 5.0f;
  out_component->enabled = true;

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

          if (strcmp (field_name, "move-speed") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->move_speed);
            }

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

result_t
parse_camera_rotation_component (scheme_state_t *state, pointer sexp,
                                 camera_rotation_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  memset (out_component, 0, sizeof (camera_rotation_component_t));
  out_component->yaw = 0.0f;
  out_component->pitch = 0.0f;
  out_component->look_sensitivity = 0.003f;
  out_component->max_pitch = 1.5f;
  out_component->min_pitch = -1.5f;
  out_component->mouse_captured = false;
  out_component->enabled = true;
  out_component->first_mouse = true;

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

          if (strcmp (field_name, "yaw") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->yaw);
            }

          else if (strcmp (field_name, "pitch") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->pitch);
            }

          else if (strcmp (field_name, "look-sensitivity") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value,
                                  &out_component->look_sensitivity);
            }

          else if (strcmp (field_name, "max-pitch") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->max_pitch);
            }

          else if (strcmp (field_name, "min-pitch") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->min_pitch);
            }

          else if (strcmp (field_name, "mouse-captured") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, value))
                out_component->mouse_captured
                    = scheme_boolean_wrapper (state, value);
            }

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

result_t
parse_developer_overlay_component (
    scheme_state_t *state, pointer sexp,
    developer_overlay_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  memset (out_component, 0, sizeof (developer_overlay_component_t));
  out_component->enabled = true;
  out_component->camera_entity = INVALID_ENTITY;
  out_component->text_element_count = 0;
  out_component->fps_update_interval = 0.5f;

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

          if (strcmp (field_name, "enabled") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_boolean_wrapper (state, value))
                out_component->enabled = scheme_boolean_wrapper (state, value);
            }

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

void
apply_component_override (scheme_state_t *state, const char *component_name,
                          pointer sexp, void *target_component)
{
  if (!state || !component_name || !sexp || !target_component)
    {
      printf ("[Component Override] Invalid arguments: state=%p, name=%s, "
              "sexp=%p, target=%p\n",
              (void *)state, component_name ? component_name : "NULL",
              (void *)sexp, target_component);
      return;
    }

  printf ("[Component Override] Applying override for component '%s'\n",
          component_name);

  if (strcmp (component_name, "shape") == 0)
    {
      apply_shape_component_override (state, sexp,
                                      (shape_component_t *)target_component);
    }
  else if (strcmp (component_name, "camera") == 0)
    {

      camera_component_t *camera = (camera_component_t *)target_component;
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

              if (strcmp (field_name, "position") == 0)
                {
                  pointer pos_list = scheme_cdr_wrapper (state, field);
                  scheme_parse_vec3 (state, pos_list, &camera->position);
                }
              else if (strcmp (field_name, "direction") == 0)
                {
                  pointer dir_list = scheme_cdr_wrapper (state, field);
                  scheme_parse_vec3 (state, dir_list, &camera->direction);
                }
              else if (strcmp (field_name, "up") == 0)
                {
                  pointer up_list = scheme_cdr_wrapper (state, field);
                  scheme_parse_vec3 (state, up_list, &camera->up);
                }
              else if (strcmp (field_name, "fov") == 0)
                {
                  pointer value = scheme_cadr_wrapper (state, field);
                  scheme_parse_float (state, value, &camera->fov);
                }
              else if (strcmp (field_name, "near-plane") == 0)
                {
                  pointer value = scheme_cadr_wrapper (state, field);
                  scheme_parse_float (state, value, &camera->near_plane);
                }
              else if (strcmp (field_name, "far-plane") == 0)
                {
                  pointer value = scheme_cadr_wrapper (state, field);
                  scheme_parse_float (state, value, &camera->far_plane);
                }
              else if (strcmp (field_name, "background-color") == 0)
                {
                  pointer color_list = scheme_cdr_wrapper (state, field);
                  scheme_parse_vec3 (state, color_list,
                                     &camera->background_color);
                }
              else if (strcmp (field_name, "active") == 0)
                {
                  pointer value = scheme_cadr_wrapper (state, field);
                  if (scheme_is_boolean_wrapper (state, value))
                    camera->is_active = scheme_boolean_wrapper (state, value);
                }
            }

          current = scheme_cdr_wrapper (state, current);
        }
    }
}

result_t
parse_lighting_component (scheme_state_t *state, pointer sexp,
                          lighting_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  *out_component = lighting_create_default ();

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

          if (strcmp (field_name, "sun-direction") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_pair_wrapper (state, value))
                {
                  scheme_parse_vec3 (state, value,
                                     &out_component->sun_direction);

                  float len = sqrtf (out_component->sun_direction.x
                                         * out_component->sun_direction.x
                                     + out_component->sun_direction.y
                                           * out_component->sun_direction.y
                                     + out_component->sun_direction.z
                                           * out_component->sun_direction.z);
                  if (len > 0.0f)
                    {
                      out_component->sun_direction.x /= len;
                      out_component->sun_direction.y /= len;
                      out_component->sun_direction.z /= len;
                    }
                }
            }

          else if (strcmp (field_name, "sun-color") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_pair_wrapper (state, value))
                {
                  scheme_parse_vec3 (state, value, &out_component->sun_color);
                }
            }

          else if (strcmp (field_name, "ambient-strength") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value,
                                  &out_component->ambient_strength);
            }

          else if (strcmp (field_name, "diffuse-strength") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value,
                                  &out_component->diffuse_strength);
            }

          else if (strcmp (field_name, "shadow-bias") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value, &out_component->shadow_bias);
            }

          else if (strcmp (field_name, "shadow-softness") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              scheme_parse_float (state, value,
                                  &out_component->shadow_softness);
            }

          else if (strcmp (field_name, "shadow-steps") == 0)
            {
              pointer value = scheme_cadr_wrapper (state, field);
              if (scheme_is_number_wrapper (state, value))
                {
                  int steps = (int)scheme_number_wrapper (state, value);
                  out_component->shadow_steps = steps;
                }
            }

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
