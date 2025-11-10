#ifndef HITE_COMPONENT_PARSERS_H
#define HITE_COMPONENT_PARSERS_H

#include "../components/camera_component.h"
#include "../components/camera_movement_component.h"
#include "../components/camera_rotation_component.h"
#include "../components/developer_overlay_component.h"
#include "../components/lighting_component.h"
#include "../components/player_collider_component.h"
#include "../components/player_component.h"
#include "../components/player_movement_component.h"
#include "../components/player_movement_controls_component.h"
#include "../components/shape_component.h"
#include "../components/transform_component.h"
#include "scheme_parser.h"
#include "types.h"

#define GET_FIELD_NAME(field)                                                 \
  pointer field_name_obj = scheme_car_wrapper (state, field);                 \
  if (!scheme_is_symbol_wrapper (state, field_name_obj))                      \
    continue;                                                                 \
  const char *field_name                                                      \
      = scheme_symbol_name_wrapper (state, field_name_obj);                   \
  if (!field_name)                                                            \
    continue;

#define PARSE_FLOAT(name, target)                                             \
  if (strcmp (field_name, name) == 0)                                         \
    {                                                                         \
      pointer value = scheme_cadr_wrapper (state, field);                     \
      scheme_parse_float (state, value, &target);                             \
    }

#define PARSE_VEC3(name, target)                                              \
  if (strcmp (field_name, name) == 0)                                         \
    {                                                                         \
      pointer value = scheme_cdr_wrapper (state, field);                      \
      scheme_parse_vec3 (state, value, &target);                              \
    }

#define PARSE_VEC4(name, target)                                              \
  if (strcmp (field_name, name) == 0)                                         \
    {                                                                         \
      pointer value = scheme_cdr_wrapper (state, field);                      \
      scheme_parse_vec4 (state, value, &target);                              \
    }

#define PARSE_BOOL(name, target)                                              \
  if (strcmp (field_name, name) == 0)                                         \
    {                                                                         \
      pointer value = scheme_cadr_wrapper (state, field);                     \
      if (scheme_is_boolean_wrapper (state, value))                           \
        target = scheme_boolean_wrapper (state, value);                       \
    }

#define PARSE_INT(name, target)                                               \
  if (strcmp (field_name, name) == 0)                                         \
    {                                                                         \
      pointer value = scheme_cadr_wrapper (state, field);                     \
      if (scheme_is_number_wrapper (state, value))                            \
        target = (int)scheme_number_wrapper (state, value);                   \
    }

result_t parse_shape_component (scheme_state_t *state, pointer sexp,
                                shape_component_t *out_component);

result_t parse_camera_component (scheme_state_t *state, pointer sexp,
                                 camera_component_t *out_component);

result_t
parse_camera_movement_component (scheme_state_t *state, pointer sexp,
                                 camera_movement_component_t *out_component);

result_t
parse_camera_rotation_component (scheme_state_t *state, pointer sexp,
                                 camera_rotation_component_t *out_component);

result_t parse_developer_overlay_component (
    scheme_state_t *state, pointer sexp,
    developer_overlay_component_t *out_component);

result_t parse_lighting_component (scheme_state_t *state, pointer sexp,
                                   lighting_component_t *out_component);

result_t parse_transform_component (scheme_state_t *state, pointer sexp,
                                    transform_component_t *out_component);

result_t parse_player_component (scheme_state_t *state, pointer sexp,
                                 player_component_t *out_component);

result_t
parse_player_collider_component (scheme_state_t *state, pointer sexp,
                                 player_collider_component_t *out_component);

result_t parse_player_movement_controls_component (
    scheme_state_t *state, pointer sexp,
    player_movement_controls_component_t *out_component);

result_t
parse_player_movement_component (scheme_state_t *state, pointer sexp,
                                 player_movement_component_t *out_component);

void apply_component_override (scheme_state_t *state,
                               const char *component_name, pointer sexp,
                               void *target_component);

#endif
