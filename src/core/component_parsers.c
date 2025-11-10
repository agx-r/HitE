#include "component_parsers.h"
#include "../components/lighting_component.h"
#include "../components/shape_component.h"
#include "logger.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static pointer
component_fields_cursor (scheme_state_t *state, pointer sexp)
{
  pointer current = scheme_cdr_wrapper (state, sexp);
  if (scheme_is_pair_wrapper (state, current))
    current = scheme_cdr_wrapper (state, current);
  return current;
}

static void
shape_component_set_defaults (shape_component_t *component)
{
  memset (component, 0, sizeof (*component));
  component->type = SHAPE_SPHERE;
  component->operation = SHAPE_OP_UNION;
  component->dimensions = (vec3_t){ 1.0f, 1.0f, 1.0f, 0.0f };
  component->size = 1.0f;
  component->color = (vec4_t){ 1.0f, 1.0f, 1.0f, 1.0f };
  component->transform.position = (vec3_t){ 0.0f, 0.0f, 0.0f, 0.0f };
  component->transform.scale = (vec3_t){ 1.0f, 1.0f, 1.0f, 0.0f };
  component->transform.rotation = (vec4_t){ 0.0f, 0.0f, 0.0f, 1.0f };
  component->visible = true;
  component->dirty = true;
}

static result_t
parse_shape_type (scheme_state_t *state, pointer sexp, shape_type_t *out_type)
{
  if (!state || !sexp || !out_type)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  if (scheme_is_number_wrapper (state, sexp))
    {
      int value = (int)scheme_number_wrapper (state, sexp);
      *out_type = (shape_type_t)value;
      return RESULT_SUCCESS;
    }

  const char *type_str = NULL;
  if (scheme_is_string_wrapper (state, sexp))
    type_str = scheme_string_wrapper (state, sexp);
  else if (scheme_is_symbol_wrapper (state, sexp))
    type_str = scheme_symbol_name_wrapper (state, sexp);
  else
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Shape type must be a string, symbol, or number");

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
  else if (strcmp (type_str, "custom") == 0)
    *out_type = SHAPE_CUSTOM;
  else
    {
      char err[128];
      snprintf (err, sizeof (err), "Unknown shape type: %s", type_str);
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, err);
    }

  return RESULT_SUCCESS;
}

static result_t
parse_shape_operation (scheme_state_t *state, pointer sexp,
                       shape_operation_t *out_operation)
{
  if (!state || !sexp || !out_operation)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  if (scheme_is_number_wrapper (state, sexp))
    {
      int value = (int)scheme_number_wrapper (state, sexp);
      *out_operation = (shape_operation_t)value;
      return RESULT_SUCCESS;
    }

  const char *operation_str = NULL;
  if (scheme_is_string_wrapper (state, sexp))
    operation_str = scheme_string_wrapper (state, sexp);
  else if (scheme_is_symbol_wrapper (state, sexp))
    operation_str = scheme_symbol_name_wrapper (state, sexp);
  else
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Operation must be a string, symbol, or number");

  if (!operation_str)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Failed to get shape operation");

  if (strcmp (operation_str, "union") == 0)
    *out_operation = SHAPE_OP_UNION;
  else if (strcmp (operation_str, "subtraction") == 0)
    *out_operation = SHAPE_OP_SUBTRACTION;
  else if (strcmp (operation_str, "intersection") == 0)
    *out_operation = SHAPE_OP_INTERSECTION;
  else if (strcmp (operation_str, "smooth-union") == 0
           || strcmp (operation_str, "smooth_union") == 0)
    *out_operation = SHAPE_OP_SMOOTH_UNION;
  else
    {
      char err[128];
      snprintf (err, sizeof (err), "Unknown shape operation: %s",
                operation_str);
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, err);
    }

  return RESULT_SUCCESS;
}

static result_t
shape_component_parse_impl (scheme_state_t *state, pointer sexp,
                            shape_component_t *component, bool reset_defaults)
{
  if (!state || !sexp || !component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  if (!scheme_is_pair_wrapper (state, sexp))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Invalid component format");

  if (reset_defaults)
    shape_component_set_defaults (component);

  pointer current = component_fields_cursor (state, sexp);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);
      if (!scheme_is_pair_wrapper (state, field))
        {
          current = scheme_cdr_wrapper (state, current);
          continue;
        }

      GET_FIELD_NAME (field)

      if (strcmp (field_name, "type") == 0)
        {
          pointer type_value
              = scheme_car_wrapper (state, scheme_cdr_wrapper (state, field));
          result_t res
              = parse_shape_type (state, type_value, &component->type);
          if (res.code != RESULT_OK)
            LOG_WARNING ("Component Parser", "%s", res.message);
        }
      else if (strcmp (field_name, "operation") == 0)
        {
          pointer op_value
              = scheme_car_wrapper (state, scheme_cdr_wrapper (state, field));
          result_t res
              = parse_shape_operation (state, op_value, &component->operation);
          if (res.code != RESULT_OK)
            LOG_WARNING ("Component Parser", "%s", res.message);
        }
      else if (strcmp (field_name, "seed") == 0)
        {
          pointer value = scheme_cadr_wrapper (state, field);
          scheme_parse_float (state, value, &component->roughness);
        }
      else
        PARSE_VEC3 ("position", component->transform.position)
      else PARSE_VEC3 ("scale", component->transform.scale) else PARSE_VEC4 (
          "rotation",
          component->transform
              .rotation) else PARSE_VEC3 ("dimensions",
                                          component
                                              ->dimensions) else PARSE_FLOAT ("size",
                                                                              component
                                                                                  ->size) else PARSE_VEC4 ("color",
                                                                                                           component
                                                                                                               ->color) else PARSE_FLOAT ("roughness", component
                                                                                                                                                           ->roughness) else PARSE_FLOAT ("metallic", component
                                                                                                                                                                                                          ->metallic) else PARSE_FLOAT ("smoothing", component
                                                                                                                                                                                                                                                         ->smoothing) else PARSE_BOOL ("visible",
                                                                                                                                                                                                                                                                                       component
                                                                                                                                                                                                                                                                                           ->visible) else PARSE_INT ("gpu-index", component
                                                                                                                                                                                                                                                                                                                                       ->gpu_index) else PARSE_BOOL ("dirty",
                                                                                                                                                                                                                                                                                                                                                                     component
                                                                                                                                                                                                                                                                                                                                                                         ->dirty) else
      {
        LOG_DEBUG ("Component Parser", "Unknown shape field '%s'", field_name);
      }

      current = scheme_cdr_wrapper (state, current);
    }

  component->dirty = true;
  return RESULT_SUCCESS;
}

static void
camera_component_set_defaults (camera_component_t *component)
{
  memset (component, 0, sizeof (*component));
  component->fov = CAMERA_DEFAULT_FOV;
  component->near_plane = 0.1f;
  component->far_plane = 1000.0f;
  component->background_color = (vec3_t){ 0.06f, 0.06f, 0.06f, 0.0f };
  component->is_active = true;
}

static result_t
camera_component_parse_impl (scheme_state_t *state, pointer sexp,
                             camera_component_t *component,
                             bool reset_defaults)
{
  if (!state || !sexp || !component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  if (!scheme_is_pair_wrapper (state, sexp))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Invalid component format");

  if (reset_defaults)
    camera_component_set_defaults (component);

  pointer current = component_fields_cursor (state, sexp);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);
      if (!scheme_is_pair_wrapper (state, field))
        {
          current = scheme_cdr_wrapper (state, current);
          continue;
        }

      GET_FIELD_NAME (field)

      PARSE_FLOAT ("fov", component->fov)
      else PARSE_FLOAT ("near-plane", component->near_plane) else PARSE_FLOAT (
          "far-plane",
          component
              ->far_plane) else PARSE_BOOL ("active",
                                            component
                                                ->is_active) else PARSE_VEC3 ("background-color",
                                                                              component
                                                                                  ->background_color) else
      {
        LOG_DEBUG ("Component Parser", "Unknown camera field '%s'",
                   field_name);
      }

      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

static void
apply_shape_component_override (scheme_state_t *state, pointer sexp,
                                shape_component_t *target)
{
  if (!target)
    {
      LOG_WARNING ("Component Override", "Shape override target is NULL");
      return;
    }

  result_t res = shape_component_parse_impl (state, sexp, target,
                                             false /* keep data */);
  if (res.code != RESULT_OK)
    {
      LOG_WARNING ("Component Override", "Failed to apply shape override: %s",
                   res.message ? res.message : "unknown error");
    }
}

static void
apply_transform_component_override (scheme_state_t *state, pointer sexp,
                                    transform_component_t *target)
{
  if (!target)
    {
      LOG_WARNING ("Component Override", "Transform override target is NULL");
      return;
    }

  LOG_DEBUG ("Component Override",
             "Applying transform override (initial position: %.3f %.3f %.3f)",
             target->transform.position.x, target->transform.position.y,
             target->transform.position.z);

  pointer current = component_fields_cursor (state, sexp);
  bool any_field_applied = false;

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);
      if (!scheme_is_pair_wrapper (state, field))
        {
          current = scheme_cdr_wrapper (state, current);
          continue;
        }

      GET_FIELD_NAME (field)

      bool handled = false;

      if (strcmp (field_name, "position") == 0)
        {
          vec3_t position = target->transform.position;
          pointer value = scheme_cdr_wrapper (state, field);
          scheme_parse_vec3 (state, value, &position);
          transform_set_position (target, position);
          LOG_DEBUG ("Component Override",
                     "Transform override position -> %.3f %.3f %.3f",
                     position.x, position.y, position.z);
          any_field_applied = true;
          handled = true;
        }
      else if (strcmp (field_name, "rotation") == 0)
        {
          vec4_t rotation = target->transform.rotation;
          pointer value = scheme_cdr_wrapper (state, field);
          scheme_parse_vec4 (state, value, &rotation);
          transform_set_rotation (target, rotation);
          LOG_DEBUG ("Component Override",
                     "Transform override rotation quat -> %.3f %.3f %.3f %.3f",
                     rotation.x, rotation.y, rotation.z, rotation.w);
          any_field_applied = true;
          handled = true;
        }
      else if (strcmp (field_name, "rotation-euler") == 0)
        {
          vec3_t euler = { 0.0f, 0.0f, 0.0f, 0.0f };
          pointer value = scheme_cdr_wrapper (state, field);
          scheme_parse_vec3 (state, value, &euler);
          vec4_t quat
              = transform_quaternion_from_euler (euler.x, euler.y, euler.z);
          transform_set_rotation (target, quat);
          LOG_DEBUG ("Component Override",
                     "Transform override rotation euler -> %.3f %.3f %.3f",
                     euler.x, euler.y, euler.z);
          any_field_applied = true;
          handled = true;
        }
      else if (strcmp (field_name, "scale") == 0)
        {
          pointer value = scheme_cdr_wrapper (state, field);
          scheme_parse_vec3 (state, value, &target->transform.scale);
          target->transform.scale._padding = 0.0f;
          target->dirty = true;
          LOG_DEBUG ("Component Override",
                     "Transform override scale -> %.3f %.3f %.3f",
                     target->transform.scale.x, target->transform.scale.y,
                     target->transform.scale.z);
          any_field_applied = true;
          handled = true;
        }
      else if (strcmp (field_name, "dirty") == 0)
        {
          pointer value = scheme_cadr_wrapper (state, field);
          if (scheme_is_boolean_wrapper (state, value))
            {
              target->dirty = scheme_boolean_wrapper (state, value);
              handled = true;
            }
        }

      if (!handled)
        {
          LOG_DEBUG ("Component Override",
                     "Transform override ignored field '%s'", field_name);
        }

      current = scheme_cdr_wrapper (state, current);
    }

  LOG_DEBUG ("Component Override",
             "Transform override %sapplied (final position: %.3f %.3f %.3f)",
             any_field_applied ? "" : "NOT ", target->transform.position.x,
             target->transform.position.y, target->transform.position.z);
}

result_t
parse_shape_component (scheme_state_t *state, pointer sexp,
                       shape_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  return shape_component_parse_impl (state, sexp, out_component, true);
}

result_t
parse_camera_component (scheme_state_t *state, pointer sexp,
                        camera_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  return camera_component_parse_impl (state, sexp, out_component, true);
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
          GET_FIELD_NAME (field)

          PARSE_FLOAT ("move-speed", out_component->move_speed)
          else PARSE_BOOL ("enabled", out_component->enabled)
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
          GET_FIELD_NAME (field)

          PARSE_FLOAT ("yaw", out_component->yaw)
          else PARSE_FLOAT ("pitch", out_component->pitch) else PARSE_FLOAT (
              "look-sensitivity",
              out_component
                  ->look_sensitivity) else PARSE_FLOAT ("max-pitch",
                                                        out_component
                                                            ->max_pitch) else PARSE_FLOAT ("min-pitch",
                                                                                           out_component
                                                                                               ->min_pitch) else PARSE_BOOL ("mouse-captured",
                                                                                                                             out_component
                                                                                                                                 ->mouse_captured) else PARSE_BOOL ("enabled",
                                                                                                                                                                    out_component
                                                                                                                                                                        ->enabled)
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
          GET_FIELD_NAME (field)

          PARSE_BOOL ("enabled", out_component->enabled)
          else PARSE_FLOAT ("fps-update-interval",
                            out_component->fps_update_interval)
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
      LOG_ERROR ("Component Override",
                 "Invalid arguments: state=%p, name=%s, sexp=%p, target=%p",
                 (void *)state, component_name ? component_name : "NULL",
                 (void *)sexp, target_component);
      return;
    }

  LOG_DEBUG ("Component Override", "Applying override for component '%s'",
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
              GET_FIELD_NAME (field)

              PARSE_FLOAT ("fov", camera->fov)
              else PARSE_FLOAT ("near-plane", camera->near_plane) else PARSE_FLOAT (
                  "far-plane",
                  camera
                      ->far_plane) else PARSE_VEC3 ("background-color",
                                                    camera
                                                        ->background_color) else PARSE_BOOL ("active",
                                                                                             camera
                                                                                                 ->is_active)
            }
          current = scheme_cdr_wrapper (state, current);
        }
    }
  else if (strcmp (component_name, "transform") == 0)
    {
      apply_transform_component_override (
          state, sexp, (transform_component_t *)target_component);
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
          GET_FIELD_NAME (field)

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
                scheme_parse_vec3 (state, value, &out_component->sun_color);
            }
          else
            PARSE_FLOAT ("ambient-strength", out_component->ambient_strength)
          else PARSE_FLOAT ("diffuse-strength", out_component->diffuse_strength) else PARSE_FLOAT (
              "shadow-bias",
              out_component
                  ->shadow_bias) else PARSE_FLOAT ("shadow-softness",
                                                   out_component
                                                       ->shadow_softness) else PARSE_INT ("shadow-steps",
                                                                                          out_component
                                                                                              ->shadow_steps) else PARSE_BOOL ("enabled",
                                                                                                                               out_component
                                                                                                                                   ->enabled)
        }
      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

result_t
parse_transform_component (scheme_state_t *state, pointer sexp,
                           transform_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  memset (out_component, 0, sizeof (*out_component));
  out_component->transform.position = (vec3_t){ 0.0f, 0.0f, 0.0f, 0.0f };
  out_component->transform.rotation = (vec4_t){ 0.0f, 0.0f, 0.0f, 1.0f };
  out_component->transform.scale = (vec3_t){ 1.0f, 1.0f, 1.0f, 0.0f };
  out_component->dirty = false;

  pointer current = component_fields_cursor (state, sexp);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);
      if (!scheme_is_pair_wrapper (state, field))
        {
          current = scheme_cdr_wrapper (state, current);
          continue;
        }

      GET_FIELD_NAME (field)

      if (strcmp (field_name, "position") == 0)
        {
          vec3_t position = { 0.0f, 0.0f, 0.0f, 0.0f };
          pointer value = scheme_cdr_wrapper (state, field);
          scheme_parse_vec3 (state, value, &position);
          transform_set_position (out_component, position);
        }
      else if (strcmp (field_name, "rotation") == 0)
        {
          vec4_t rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
          pointer value = scheme_cdr_wrapper (state, field);
          scheme_parse_vec4 (state, value, &rotation);
          transform_set_rotation (out_component, rotation);
        }
      else if (strcmp (field_name, "rotation-euler") == 0)
        {
          vec3_t euler = { 0.0f, 0.0f, 0.0f, 0.0f };
          pointer value = scheme_cdr_wrapper (state, field);
          scheme_parse_vec3 (state, value, &euler);
          vec4_t quat
              = transform_quaternion_from_euler (euler.x, euler.y, euler.z);
          transform_set_rotation (out_component, quat);
        }
      else if (strcmp (field_name, "scale") == 0)
        {
          pointer value = scheme_cdr_wrapper (state, field);
          scheme_parse_vec3 (state, value, &out_component->transform.scale);
          out_component->transform.scale._padding = 0.0f;
        }
      else
        PARSE_BOOL ("dirty", out_component->dirty)

      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

result_t
parse_player_component (scheme_state_t *state, pointer sexp,
                        player_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  memset (out_component, 0, sizeof (*out_component));
  out_component->camera_entity = INVALID_ENTITY;
  out_component->active = true;

  pointer current = component_fields_cursor (state, sexp);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);
      if (!scheme_is_pair_wrapper (state, field))
        {
          current = scheme_cdr_wrapper (state, current);
          continue;
        }

      GET_FIELD_NAME (field)

      PARSE_BOOL ("active", out_component->active)
      else if (strcmp (field_name, "camera-entity") == 0)
      {
        pointer value = scheme_cadr_wrapper (state, field);
        if (scheme_is_number_wrapper (state, value))
          out_component->camera_entity
              = (entity_id_t)scheme_number_wrapper (state, value);
      }

      current = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

result_t
parse_player_collider_component (scheme_state_t *state, pointer sexp,
                                 player_collider_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  memset (out_component, 0, sizeof (*out_component));
  out_component->radius = 0.35f;
  out_component->height = 1.8f;
  out_component->skin_width = 0.05f;
  out_component->camera_height = 1.7f;
  out_component->surface_normal = (vec3_t){ 0.0f, 1.0f, 0.0f, 0.0f };
  out_component->grounded = true;

  vec3_t parsed_offset = { 0.0f, 0.0f, 0.0f, 0.0f };
  bool has_custom_offset = false;

  pointer current = component_fields_cursor (state, sexp);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);
      if (!scheme_is_pair_wrapper (state, field))
        {
          current = scheme_cdr_wrapper (state, current);
          continue;
        }

      GET_FIELD_NAME (field)

      if (strcmp (field_name, "radius") == 0)
        {
          pointer value = scheme_cadr_wrapper (state, field);
          scheme_parse_float (state, value, &out_component->radius);
        }
      else if (strcmp (field_name, "height") == 0)
        {
          pointer value = scheme_cadr_wrapper (state, field);
          scheme_parse_float (state, value, &out_component->height);
        }
      else if (strcmp (field_name, "skin-width") == 0)
        {
          pointer value = scheme_cadr_wrapper (state, field);
          scheme_parse_float (state, value, &out_component->skin_width);
        }
      else if (strcmp (field_name, "camera-height") == 0)
        {
          pointer value = scheme_cadr_wrapper (state, field);
          scheme_parse_float (state, value, &out_component->camera_height);
        }
      else if (strcmp (field_name, "offset") == 0)
        {
          pointer value = scheme_cdr_wrapper (state, field);
          scheme_parse_vec3 (state, value, &parsed_offset);
          has_custom_offset = true;
        }
      else if (strcmp (field_name, "surface-normal") == 0)
        {
          pointer value = scheme_cdr_wrapper (state, field);
          scheme_parse_vec3 (state, value, &out_component->surface_normal);
        }
      else
        PARSE_BOOL ("grounded", out_component->grounded)

      current = scheme_cdr_wrapper (state, current);
    }

  player_collider_recalculate_offset (out_component);
  if (has_custom_offset)
    {
      out_component->offset.x += parsed_offset.x;
      out_component->offset.y += parsed_offset.y;
      out_component->offset.z += parsed_offset.z;
      out_component->offset._padding = 0.0f;
    }

  return RESULT_SUCCESS;
}

result_t
parse_player_movement_controls_component (
    scheme_state_t *state, pointer sexp,
    player_movement_controls_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  memset (out_component, 0, sizeof (*out_component));
  out_component->enabled = true;
  out_component->auto_emit = true;
  out_component->look_blend = 1.0f;

  pointer current = component_fields_cursor (state, sexp);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);
      if (!scheme_is_pair_wrapper (state, field))
        {
          current = scheme_cdr_wrapper (state, current);
          continue;
        }

      GET_FIELD_NAME (field)

      PARSE_BOOL ("enabled", out_component->enabled)
      else PARSE_BOOL (
          "auto-emit",
          out_component->auto_emit) else PARSE_FLOAT ("look-blend",
                                                      out_component
                                                          ->look_blend)

          current
          = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}

result_t
parse_player_movement_component (scheme_state_t *state, pointer sexp,
                                 player_movement_component_t *out_component)
{
  if (!state || !sexp || !out_component)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  memset (out_component, 0, sizeof (*out_component));
  out_component->velocity = (vec3_t){ 0.0f, 0.0f, 0.0f, 0.0f };
  out_component->input_direction = (vec3_t){ 0.0f, 0.0f, 0.0f, 0.0f };
  out_component->grounded = true;
  out_component->jump_requested = false;
  out_component->sprinting = false;
  out_component->input_received = false;
  out_component->move_listener = 0;
  out_component->event_system = NULL;

  pointer current = component_fields_cursor (state, sexp);

  while (scheme_is_pair_wrapper (state, current))
    {
      pointer field = scheme_car_wrapper (state, current);
      if (!scheme_is_pair_wrapper (state, field))
        {
          current = scheme_cdr_wrapper (state, current);
          continue;
        }

      GET_FIELD_NAME (field)

      PARSE_VEC3 ("velocity", out_component->velocity)
      else PARSE_BOOL ("grounded", out_component->grounded)

          current
          = scheme_cdr_wrapper (state, current);
    }

  return RESULT_SUCCESS;
}
