#include "scheme_parser.h"
#include "../../external/s7/s7.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize S7 Scheme interpreter
scheme_state_t *
scheme_init (void)
{
  scheme_state_t *state = calloc (1, sizeof (scheme_state_t));
  if (!state)
    return NULL;

  state->sc = s7_init ();
  if (!state->sc)
    {
      free (state);
      return NULL;
    }

  printf ("[S7 Scheme] Initialized (version %s)\n", S7_VERSION);

  return state;
}

// Shutdown S7 Scheme interpreter
void
scheme_shutdown (scheme_state_t *state)
{
  if (!state)
    return;

  if (state->sc)
    {
      // S7 doesn't have explicit shutdown, memory is managed internally
      // Just free our wrapper
    }

  free (state);
}

// Load and evaluate Scheme file
result_t
scheme_load_file (scheme_state_t *state, const char *filepath,
                  s7_pointer *out_result)
{
  if (!state || !state->sc || !filepath)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  // Use s7_load to load the file
  // But we want to READ, not EVAL, so we'll read the file and use s7_read
  FILE *file = fopen (filepath, "r");
  if (!file)
    {
      char err[256];
      snprintf (err, sizeof (err), "Failed to open file: %s", filepath);
      return RESULT_ERROR (RESULT_ERROR_FILE_NOT_FOUND, err);
    }

  fseek (file, 0, SEEK_END);
  long size = ftell (file);
  fseek (file, 0, SEEK_SET);

  char *source = malloc (size + 1);
  if (!source)
    {
      fclose (file);
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION, "Failed to allocate memory");
    }

  size_t bytes_read = fread (source, 1, size, file);
  source[bytes_read] = '\0';
  fclose (file);

  // Create a string port and read from it (don't evaluate)
  s7_pointer str_port = s7_open_input_string (state->sc, source);
  s7_pointer result = s7_read (state->sc, str_port);
  s7_close_input_port (state->sc, str_port);
  
  free (source);

  if (out_result)
    *out_result = result;

  return RESULT_SUCCESS;
}

// Evaluate Scheme string
result_t
scheme_eval_string (scheme_state_t *state, const char *source,
                    s7_pointer *out_result)
{
  if (!state || !state->sc || !source)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  s7_pointer result = s7_eval_c_string (state->sc, source);

  // S7 doesn't return NULL on error, it returns an error object
  // We'll just return the result as-is

  if (out_result)
    *out_result = result;

  return RESULT_SUCCESS;
}

// Type checking wrappers
bool
s7_is_list_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_is_list (state->sc, obj);
}

bool
s7_is_pair_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_is_pair (obj);
}

bool
s7_is_symbol_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_is_symbol (obj);
}

bool
s7_is_string_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_is_string (obj);
}

bool
s7_is_number_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_is_number (obj);
}

bool
s7_is_boolean_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_is_boolean (obj);
}

bool
s7_is_null_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_is_null (state->sc, obj);
}

// Value extraction wrappers
const char *
s7_symbol_name_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_symbol_name (obj);
}

const char *
s7_string_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_string (obj);
}

double
s7_number_wrapper (scheme_state_t *state, s7_pointer obj)
{
  if (s7_is_real (obj))
    return s7_real (obj);
  if (s7_is_integer (obj))
    return (double)s7_integer (obj);
  return 0.0;
}

bool
s7_boolean_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_boolean (state->sc, obj);
}

// List operations wrappers
s7_pointer
s7_car_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_car (obj);
}

s7_pointer
s7_cdr_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_cdr (obj);
}

s7_pointer
s7_cadr_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_cadr (obj);
}

int
s7_list_length_wrapper (scheme_state_t *state, s7_pointer obj)
{
  return s7_list_length (state->sc, obj);
}

s7_pointer
s7_list_ref_wrapper (scheme_state_t *state, s7_pointer obj, int index)
{
  return s7_list_ref (state->sc, obj, index);
}

// Find field in list like (field-name value...)
s7_pointer
s7_find_field (scheme_state_t *state, s7_pointer list,
               const char *field_name)
{
  if (!state || !list || !field_name)
    return NULL;

  // Iterate through list
  s7_pointer current = list;
  while (s7_is_pair (current))
    {
      s7_pointer elem = s7_car (current);

      // Check if element is a list starting with field_name
      if (s7_is_pair (elem))
        {
          s7_pointer name = s7_car (elem);
          if (s7_is_symbol (name))
            {
              const char *sym_name = s7_symbol_name (name);
              if (strcmp (sym_name, field_name) == 0)
                {
                  return elem; // Return entire field list
                }
            }
        }

      current = s7_cdr (current);
    }

  return NULL;
}

// Parse vec3 from list like (x y z)
result_t
s7_parse_vec3 (scheme_state_t *state, s7_pointer obj, vec3_t *out)
{
  if (!state || !obj || !out)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  if (!s7_is_pair (obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected list for vec3");

  int len = s7_list_length (state->sc, obj);
  if (len < 3)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "vec3 requires 3 values");

  s7_pointer x_obj = s7_car (obj);
  s7_pointer y_obj = s7_cadr (obj);
  s7_pointer z_obj = s7_list_ref (state->sc, obj, 2);

  if (!s7_is_number (x_obj) || !s7_is_number (y_obj) || !s7_is_number (z_obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "vec3 values must be numbers");

  out->x = (float)s7_number_wrapper (state, x_obj);
  out->y = (float)s7_number_wrapper (state, y_obj);
  out->z = (float)s7_number_wrapper (state, z_obj);
  out->_padding = 0.0f;

  return RESULT_SUCCESS;
}

// Parse vec4 from list like (r g b a)
result_t
s7_parse_vec4 (scheme_state_t *state, s7_pointer obj, vec4_t *out)
{
  if (!state || !obj || !out)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  if (!s7_is_pair (obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected list for vec4");

  int len = s7_list_length (state->sc, obj);
  if (len < 4)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "vec4 requires 4 values");

  s7_pointer x_obj = s7_car (obj);
  s7_pointer y_obj = s7_cadr (obj);
  s7_pointer z_obj = s7_list_ref (state->sc, obj, 2);
  s7_pointer w_obj = s7_list_ref (state->sc, obj, 3);

  if (!s7_is_number (x_obj) || !s7_is_number (y_obj) || !s7_is_number (z_obj)
      || !s7_is_number (w_obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "vec4 values must be numbers");

  out->x = (float)s7_number_wrapper (state, x_obj);
  out->y = (float)s7_number_wrapper (state, y_obj);
  out->z = (float)s7_number_wrapper (state, z_obj);
  out->w = (float)s7_number_wrapper (state, w_obj);

  return RESULT_SUCCESS;
}

// Parse float from number
result_t
s7_parse_float (scheme_state_t *state, s7_pointer obj, float *out)
{
  if (!state || !obj || !out)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  if (!s7_is_number (obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected number for float");

  *out = (float)s7_number_wrapper (state, obj);
  return RESULT_SUCCESS;
}

// Parse bool from boolean
result_t
s7_parse_bool (scheme_state_t *state, s7_pointer obj, bool *out)
{
  if (!state || !obj || !out)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  if (!s7_is_boolean (obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected boolean for bool");

  *out = s7_boolean_wrapper (state, obj);
  return RESULT_SUCCESS;
}
