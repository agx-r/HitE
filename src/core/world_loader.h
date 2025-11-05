#ifndef HITE_WORLD_LOADER_H
#define HITE_WORLD_LOADER_H

#include "types.h"
#include "world.h"

// Load world definition from file
result_t world_load_from_file (const char *filepath,
                               world_definition_t *out_definition);

// Free world definition loaded from file
void world_definition_free (world_definition_t *definition);

#endif // HITE_WORLD_LOADER_H
