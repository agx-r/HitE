#ifndef HITE_WORLD_LOADER_H
#define HITE_WORLD_LOADER_H

#include "scheme_parser.h"
#include "types.h"
#include "world.h"

result_t world_load_from_file (const char *filepath,
                               world_definition_t *out_definition);

void world_definition_free (world_definition_t *definition);

scheme_state_t *world_loader_get_scheme_state (void);

result_t world_loader_parse_component (scheme_state_t *state,
                                       pointer comp_sexp, void **out_data,
                                       size_t *out_size);

#endif
