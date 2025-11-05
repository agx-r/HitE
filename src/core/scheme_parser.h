#ifndef HITE_SCHEME_PARSER_H
#define HITE_SCHEME_PARSER_H

#include "types.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct scheme scheme;
typedef struct cell *pointer;

typedef struct
{
  scheme *sc;
} scheme_state_t;

scheme_state_t *hite_scheme_init (void);

void hite_scheme_shutdown (scheme_state_t *state);

result_t hite_scheme_load_file (scheme_state_t *state, const char *filepath,
                                pointer *out_result);

result_t hite_scheme_eval_string (scheme_state_t *state, const char *source,
                                  pointer *out_result);

bool scheme_is_list_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_pair_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_symbol_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_string_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_number_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_boolean_wrapper (scheme_state_t *state, pointer obj);
bool scheme_is_null_wrapper (scheme_state_t *state, pointer obj);

const char *scheme_symbol_name_wrapper (scheme_state_t *state, pointer obj);
const char *scheme_string_wrapper (scheme_state_t *state, pointer obj);
double scheme_number_wrapper (scheme_state_t *state, pointer obj);
bool scheme_boolean_wrapper (scheme_state_t *state, pointer obj);

pointer scheme_car_wrapper (scheme_state_t *state, pointer obj);
pointer scheme_cdr_wrapper (scheme_state_t *state, pointer obj);
pointer scheme_cadr_wrapper (scheme_state_t *state, pointer obj);
int scheme_list_length_wrapper (scheme_state_t *state, pointer obj);
pointer scheme_list_ref_wrapper (scheme_state_t *state, pointer obj,
                                 int index);

pointer scheme_find_field (scheme_state_t *state, pointer list,
                           const char *field_name);

result_t scheme_parse_vec3 (scheme_state_t *state, pointer obj, vec3_t *out);
result_t scheme_parse_vec4 (scheme_state_t *state, pointer obj, vec4_t *out);
result_t scheme_parse_float (scheme_state_t *state, pointer obj, float *out);
result_t scheme_parse_bool (scheme_state_t *state, pointer obj, bool *out);

#endif
