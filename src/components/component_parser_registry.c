#include "component_parser_registry.h"
#include "../core/component_parsers.h"
#include <stdbool.h>
#include <string.h>

static void apply_camera_override_wrapper (scheme_state_t *state, pointer sexp,
                                           void *target);

static struct
{
  const char *name;
  component_parser_fn parser;
  component_override_fn override;
} component_registry[]
    = { { "camera", (component_parser_fn)parse_camera_component,
          apply_camera_override_wrapper },
        { "camera_movement",
          (component_parser_fn)parse_camera_movement_component, NULL },
        { "camera_rotation",
          (component_parser_fn)parse_camera_rotation_component, NULL },
        { "lighting", (component_parser_fn)parse_lighting_component, NULL },
        { "shape", (component_parser_fn)parse_shape_component, NULL },
        { "developer_overlay",
          (component_parser_fn)parse_developer_overlay_component, NULL },
        { NULL, NULL, NULL } };

static void
apply_camera_override_wrapper (scheme_state_t *state, pointer sexp,
                               void *target)
{

  (void)state;
  (void)sexp;
  (void)target;
}
