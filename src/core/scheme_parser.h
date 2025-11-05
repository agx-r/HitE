#ifndef HITE_SCHEME_PARSER_H
#define HITE_SCHEME_PARSER_H

#include "types.h"
#include <stdbool.h>
#include <stddef.h>

// Forward declare s7_scheme and s7_pointer from S7
typedef struct s7_scheme s7_scheme;
typedef struct s7_cell *s7_pointer;

// S7 Scheme wrapper for the engine
typedef struct
{
  s7_scheme *sc;
} scheme_state_t;

// Initialize S7 Scheme interpreter
scheme_state_t *scheme_init (void);

// Shutdown S7 Scheme interpreter
void scheme_shutdown (scheme_state_t *state);

// Load and evaluate Scheme file, return the result
result_t scheme_load_file (scheme_state_t *state, const char *filepath,
                           s7_pointer *out_result);

// Evaluate Scheme string, return the result
result_t scheme_eval_string (scheme_state_t *state, const char *source,
                             s7_pointer *out_result);

// Helper functions for working with S7 objects

// Type checking
bool s7_is_list_wrapper (scheme_state_t *state, s7_pointer obj);
bool s7_is_pair_wrapper (scheme_state_t *state, s7_pointer obj);
bool s7_is_symbol_wrapper (scheme_state_t *state, s7_pointer obj);
bool s7_is_string_wrapper (scheme_state_t *state, s7_pointer obj);
bool s7_is_number_wrapper (scheme_state_t *state, s7_pointer obj);
bool s7_is_boolean_wrapper (scheme_state_t *state, s7_pointer obj);
bool s7_is_null_wrapper (scheme_state_t *state, s7_pointer obj);

// Value extraction
const char *s7_symbol_name_wrapper (scheme_state_t *state, s7_pointer obj);
const char *s7_string_wrapper (scheme_state_t *state, s7_pointer obj);
double s7_number_wrapper (scheme_state_t *state, s7_pointer obj);
bool s7_boolean_wrapper (scheme_state_t *state, s7_pointer obj);

// List operations
s7_pointer s7_car_wrapper (scheme_state_t *state, s7_pointer obj);
s7_pointer s7_cdr_wrapper (scheme_state_t *state, s7_pointer obj);
s7_pointer s7_cadr_wrapper (scheme_state_t *state, s7_pointer obj);
int s7_list_length_wrapper (scheme_state_t *state, s7_pointer obj);
s7_pointer s7_list_ref_wrapper (scheme_state_t *state, s7_pointer obj,
                                int index);

// Find field in list like (field-name value...)
s7_pointer s7_find_field (scheme_state_t *state, s7_pointer list,
                          const char *field_name);

// Helper parsers for common data types
result_t s7_parse_vec3 (scheme_state_t *state, s7_pointer obj, vec3_t *out);
result_t s7_parse_vec4 (scheme_state_t *state, s7_pointer obj, vec4_t *out);
result_t s7_parse_float (scheme_state_t *state, s7_pointer obj, float *out);
result_t s7_parse_bool (scheme_state_t *state, s7_pointer obj, bool *out);

#endif // HITE_SCHEME_PARSER_H
