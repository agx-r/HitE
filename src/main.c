#include "components/camera_component.h"
#include "components/camera_movement_component.h"
#include "components/camera_rotation_component.h"
#include "core/global.h"
#include "core/prefab.h"

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

// Helper to find camera entity with movement/rotation components
static entity_id_t
find_camera_entity (ecs_world_t *world)
{
  if (!world)
    return INVALID_ENTITY;

  entity_id_t camera_entity;
  camera_find_active (world, &camera_entity);
  return camera_entity;
}

// GLFW callback wrappers that forward to camera components
static void
key_callback_with_camera (GLFWwindow *window, int key, int scancode, int action,
                          int mods)
{
  (void)scancode;

  engine_state_t *state = (engine_state_t *)glfwGetWindowUserPointer (window);

  // Forward to base callback
  glfw_key_callback (window, key, scancode, action, mods);

  // Forward to camera movement component
  if (state->world_manager && state->world_manager->active_world)
    {
      entity_id_t camera_entity
          = find_camera_entity (state->world_manager->active_world);
      if (camera_entity != INVALID_ENTITY)
        {
          component_id_t movement_id
              = ecs_get_component_id (state->world_manager->active_world,
                                      "camera_movement");
          if (movement_id != INVALID_ENTITY)
            {
              camera_movement_component_t *movement
                  = (camera_movement_component_t *)ecs_get_component (
                      state->world_manager->active_world, camera_entity,
                      movement_id);
              if (movement)
                {
                  camera_movement_process_key (movement, key, action);
                }
            }
        }
    }
}

static void
mouse_callback_with_camera (GLFWwindow *window, double xpos, double ypos)
{
  engine_state_t *state = (engine_state_t *)glfwGetWindowUserPointer (window);

  // Forward to base callback (for backward compatibility)
  glfw_mouse_callback (window, xpos, ypos);

  // Forward to camera rotation component
  if (state->world_manager && state->world_manager->active_world)
    {
      entity_id_t camera_entity
          = find_camera_entity (state->world_manager->active_world);
      if (camera_entity != INVALID_ENTITY)
        {
          component_id_t rotation_id
              = ecs_get_component_id (state->world_manager->active_world,
                                      "camera_rotation");
          if (rotation_id != INVALID_ENTITY)
            {
              camera_rotation_component_t *rotation
                  = (camera_rotation_component_t *)ecs_get_component (
                      state->world_manager->active_world, camera_entity,
                      rotation_id);
              if (rotation)
                {
                  camera_rotation_process_mouse (rotation, xpos, ypos);
                }
            }
        }
    }
}

int
main (int argc, char **argv)
{
  (void)argc;
  (void)argv;

  printf ("=== HitE ===\n");
  printf ("High-performance Vulkan raymarching engine\n\n");

  // Engine configuration
  engine_config_t config = engine_config_default ();

  engine_state_t state;
  prefab_system_t *prefab_system = NULL;

  // Initialize engine
  result_t result = engine_init (&state, &config);
  if (result.code != RESULT_OK)
    {
      fprintf (stderr, "[Error] Engine initialization failed: %s\n",
               result.message);
      engine_cleanup (&state);
      return 1;
    }

  // Override callbacks to include camera component forwarding
  glfwSetKeyCallback (state.window, key_callback_with_camera);
  glfwSetCursorPosCallback (state.window, mouse_callback_with_camera);

  // Load world and prefabs
  result = engine_load_world (&state, &config, &prefab_system);
  if (result.code != RESULT_OK)
    {
      fprintf (stderr, "[Error] Failed to load world: %s\n", result.message);
      if (prefab_system)
        prefab_system_destroy (prefab_system);
      engine_cleanup (&state);
      return 1;
    }

  // Run main loop
  engine_run (&state);

  // Cleanup
  if (prefab_system)
    prefab_system_destroy (prefab_system);
  engine_cleanup (&state);

  printf ("\n[Engine] Bye\n");
  return 0;
}
