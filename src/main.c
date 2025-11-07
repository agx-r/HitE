#include "core/global.h"
#include "core/logger.h"
#include "core/prefab.h"

#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  (void)argc;
  (void)argv;

  logger_init ();
  logger_set_level (LOG_LEVEL_DEBUG);

  engine_config_t config = engine_config_default ();

  engine_state_t state;
  prefab_system_t *prefab_system = NULL;

  result_t result = engine_init (&state, &config);
  if (result.code != RESULT_OK)
    {
      LOG_ERROR ("Main", "Engine initialization failed: %s", result.message);
      engine_cleanup (&state);
      logger_shutdown ();
      return 1;
    }

  result = engine_load_world (&state, &config, &prefab_system);
  if (result.code != RESULT_OK)
    {
      LOG_ERROR ("Main", "Failed to load world: %s", result.message);
      if (prefab_system)
        prefab_system_destroy (prefab_system);
      engine_cleanup (&state);
      logger_shutdown ();
      return 1;
    }

  engine_run (&state);

  if (prefab_system)
    prefab_system_destroy (prefab_system);
  engine_cleanup (&state);

  LOG_INFO ("Main", "Bye");
  logger_shutdown ();
  return 0;
}
