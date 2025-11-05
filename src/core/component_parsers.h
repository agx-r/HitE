#ifndef HITE_COMPONENT_PARSERS_H
#define HITE_COMPONENT_PARSERS_H

#include "../components/camera_component.h"
#include "../components/camera_movement_component.h"
#include "../components/camera_rotation_component.h"
#include "../components/developer_overlay_component.h"
#include "../components/gui_component.h"
#include "../components/lighting_component.h"
#include "../components/shape_component.h"
#include "scheme_parser.h"
#include "types.h"

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

result_t parse_gui_component (scheme_state_t *state, pointer sexp,
                              gui_component_t *out_component);

result_t parse_developer_overlay_component (
    scheme_state_t *state, pointer sexp,
    developer_overlay_component_t *out_component);

result_t parse_lighting_component (scheme_state_t *state, pointer sexp,
                                   lighting_component_t *out_component);

result_t parse_shape_type (scheme_state_t *state, pointer sexp,
                           shape_type_t *out_type);

void apply_component_override (scheme_state_t *state,
                               const char *component_name, pointer sexp,
                               void *target_component);

#endif
