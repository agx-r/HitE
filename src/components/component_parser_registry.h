#ifndef HITE_COMPONENT_PARSER_REGISTRY_H
#define HITE_COMPONENT_PARSER_REGISTRY_H

#include "../core/scheme_parser.h"
#include "../core/types.h"

typedef struct cell *pointer;

typedef result_t (*component_parser_fn) (scheme_state_t *state, pointer sexp,
                                         void *out_component);

typedef void (*component_override_fn) (scheme_state_t *state, pointer sexp,
                                       void *target_component);

component_parser_fn get_component_parser (const char *component_name);

component_override_fn get_component_override (const char *component_name);

bool component_has_parser (const char *component_name);

#endif
