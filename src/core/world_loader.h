#ifndef HITE_WORLD_LOADER_H
#define HITE_WORLD_LOADER_H

#include "scheme_parser.h"
#include "types.h"
#include "world.h"

// Load world definition from file
result_t world_load_from_file (const char *filepath,
                               world_definition_t *out_definition);

// Free world definition loaded from file
void world_definition_free (world_definition_t *definition);

// Get the scheme state used for world loading (for applying overrides)
scheme_state_t *world_loader_get_scheme_state (void);

#endif // HITE_WORLD_LOADER_H
