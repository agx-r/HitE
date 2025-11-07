#include "scheme_parser.h"
#include "../../external/tinyscheme/scheme-private.h"
#include "../../external/tinyscheme/scheme.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

scheme_state_t *
hite_scheme_init (void)
{
  scheme_state_t *state = calloc (1, sizeof (scheme_state_t));
  if (!state)
    return NULL;

  state->sc = scheme_init_new ();
  if (!state->sc)
    {
      free (state);
      return NULL;
    }

  scheme_set_input_port_file (state->sc, stdin);
  scheme_set_output_port_file (state->sc, stdout);

  LOG_INFO ("TinyScheme", "Initialized (version 1.42)");

  return state;
}

void
hite_scheme_shutdown (scheme_state_t *state)
{
  if (!state)
    return;

  if (state->sc)
    {
      scheme_deinit (state->sc);
      free (state->sc);
    }

  free (state);
}

result_t
hite_scheme_load_file (scheme_state_t *state, const char *filepath,
                       pointer *out_result)
{
  if (!state || !state->sc || !filepath)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

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
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Failed to allocate memory");
    }

  size_t bytes_read = fread (source, 1, size, file);
  source[bytes_read] = '\0';
  fclose (file);

  char *wrapper = malloc (strlen (source) + 20);
  if (!wrapper)
    {
      free (source);
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Failed to allocate memory");
    }

  sprintf (wrapper, "(quote %s)", source);

  scheme_load_string (state->sc, wrapper);

  pointer result = state->sc->value;

  free (wrapper);
  free (source);

  if (out_result)
    *out_result = result;

  return RESULT_SUCCESS;
}

bool
scheme_is_pair_wrapper (scheme_state_t *state, pointer obj)
{
  (void)state;
  return is_pair (obj);
}

bool
scheme_is_symbol_wrapper (scheme_state_t *state, pointer obj)
{
  (void)state;
  return is_symbol (obj);
}

bool
scheme_is_string_wrapper (scheme_state_t *state, pointer obj)
{
  (void)state;
  return is_string (obj);
}

bool
scheme_is_number_wrapper (scheme_state_t *state, pointer obj)
{
  (void)state;
  return is_number (obj);
}

bool
scheme_is_boolean_wrapper (scheme_state_t *state, pointer obj)
{
  if (!state || !obj)
    return false;

  return (obj == state->sc->T || obj == state->sc->F);
}

const char *
scheme_symbol_name_wrapper (scheme_state_t *state, pointer obj)
{
  (void)state;
  return symname (obj);
}

const char *
scheme_string_wrapper (scheme_state_t *state, pointer obj)
{
  (void)state;
  return string_value (obj);
}

double
scheme_number_wrapper (scheme_state_t *state, pointer obj)
{
  (void)state;
  if (is_real (obj))
    return rvalue (obj);
  if (is_integer (obj))
    return (double)ivalue (obj);
  return 0.0;
}

bool
scheme_boolean_wrapper (scheme_state_t *state, pointer obj)
{
  if (!state || !obj)
    return false;

  return (obj != state->sc->F);
}

pointer
scheme_car_wrapper (scheme_state_t *state, pointer obj)
{
  (void)state;
  return pair_car (obj);
}

pointer
scheme_cdr_wrapper (scheme_state_t *state, pointer obj)
{
  (void)state;
  return pair_cdr (obj);
}

pointer
scheme_cadr_wrapper (scheme_state_t *state, pointer obj)
{
  return pair_car (pair_cdr (obj));
}

static pointer
scheme_list_ref_wrapper (scheme_state_t *state, pointer obj, int index)
{
  pointer current = obj;
  for (int i = 0; i < index && is_pair (current); i++)
    {
      current = pair_cdr (current);
    }

  if (is_pair (current))
    return pair_car (current);

  return state->sc->NIL;
}

result_t
scheme_parse_vec3 (scheme_state_t *state, pointer obj, vec3_t *out)
{
  if (!state || !obj || !out)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  if (!is_pair (obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected list for vec3");

  int len = list_length (state->sc, obj);
  if (len < 3)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "vec3 requires 3 values");

  pointer x_obj = pair_car (obj);
  pointer y_obj = scheme_cadr_wrapper (state, obj);
  pointer z_obj = scheme_list_ref_wrapper (state, obj, 2);

  if (!is_number (x_obj) || !is_number (y_obj) || !is_number (z_obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "vec3 values must be numbers");

  out->x = (float)scheme_number_wrapper (state, x_obj);
  out->y = (float)scheme_number_wrapper (state, y_obj);
  out->z = (float)scheme_number_wrapper (state, z_obj);
  out->_padding = 0.0f;

  return RESULT_SUCCESS;
}

result_t
scheme_parse_vec4 (scheme_state_t *state, pointer obj, vec4_t *out)
{
  if (!state || !obj || !out)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  if (!is_pair (obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected list for vec4");

  int len = list_length (state->sc, obj);
  if (len < 4)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "vec4 requires 4 values");

  pointer x_obj = pair_car (obj);
  pointer y_obj = scheme_cadr_wrapper (state, obj);
  pointer z_obj = scheme_list_ref_wrapper (state, obj, 2);
  pointer w_obj = scheme_list_ref_wrapper (state, obj, 3);

  if (!is_number (x_obj) || !is_number (y_obj) || !is_number (z_obj)
      || !is_number (w_obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "vec4 values must be numbers");

  out->x = (float)scheme_number_wrapper (state, x_obj);
  out->y = (float)scheme_number_wrapper (state, y_obj);
  out->z = (float)scheme_number_wrapper (state, z_obj);
  out->w = (float)scheme_number_wrapper (state, w_obj);

  return RESULT_SUCCESS;
}

result_t
scheme_parse_float (scheme_state_t *state, pointer obj, float *out)
{
  if (!state || !obj || !out)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid arguments");

  if (!is_number (obj))
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                         "Expected number for float");

  *out = (float)scheme_number_wrapper (state, obj);
  return RESULT_SUCCESS;
}
