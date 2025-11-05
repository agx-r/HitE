#ifndef HITE_SCHEME_PARSER_H
#define HITE_SCHEME_PARSER_H

#include "types.h"
#include <stdbool.h>
#include <stddef.h>

// Forward declare scheme and pointer from TinyScheme
typedef struct scheme scheme;
typedef struct cell *pointer;

// TinyScheme wrapper for the engine
typedef struct
{
  scheme *sc;
} scheme_state_t;

// Initialize TinyScheme interpreter
scheme_state_t *hite_scheme_init (void);

// Shutdown TinyScheme interpreter
void hite_scheme_shutdown (scheme_state_t *state);

// Load and evaluate Scheme file, return the result
result_t hite_scheme_load_file (scheme_state_t *state, const char *filepath,
                                pointer *out_result);

// Evaluate Scheme string, return the result
result_t hite_scheme_eval_string (scheme_state_t *state, const char *source,
                                  pointer *out_result);

// Helper functions for working with TinyScheme objects

// Type checking
bool scheme_is_list_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_pair_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_symbol_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_string_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_number_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_boolean_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_null_wrapper (scheme_state_t *state, pointer obj);

// Value extraction
const char *scheme_symbol_name_wrapper (scheme_state_t *state, pointer obj);
const char *scheme_string_wrapper (scheme_state_t *state, pointer obj);
double scheme_number_wrapper (scheme_state_t *state, pointer obj);
bool scheme_boolean_wrapper (scheme_state_t *state, pointer obj);

// List operations
pointer scheme_car_wrapper (scheme_state_t *state, pointer obj);
pointer scheme_cdr_wrapper (scheme_state_t *state, pointer obj);
pointer scheme_cadr_wrapper (scheme_state_t *state, pointer obj);
int scheme_list_length_wrapper (scheme_state_t *state, pointer obj);
pointer scheme_list_ref_wrapper (scheme_state_t *state, pointer obj,
                                int index);

// Find field in list like (field-name value...)
pointer scheme_find_field (scheme_state_t *state, pointer list,
                          const char *field_name);

// Helper parsers for common data types
result_t scheme_parse_vec3 (scheme_state_t *state, pointer obj, vec3_t *out);
result_t scheme_parse_vec4 (scheme_state_t *state, pointer obj, vec4_t *out);
result_t scheme_parse_float (scheme_state_t *state, pointer obj, float *out);
result_t scheme_parse_bool (scheme_state_t *state, pointer obj, bool *out);

#endif // HITE_SCHEME_PARSER_H
