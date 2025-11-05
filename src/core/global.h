#ifndef HITE_GLOBAL_H
#define HITE_GLOBAL_H

#include "events.h"
#include "resources.h"
#include "types.h"
#include "world.h"
#include "../renderer/render_system.h"
#include "../renderer/vulkan_core.h"

#include <GLFW/glfw3.h>
#include <stdbool.h>

// Global engine state
typedef struct
{
  // Core subsystems
  GLFWwindow *window;
  vulkan_context_t vk_context;
  render_system_t render_system;
  world_manager_t *world_manager;
  event_system_t *event_system;
  resource_manager_t *resource_manager;

  // Engine state
  bool running;
  double last_time;

  // Camera control (TODO: will be moved to camera component)
  bool first_mouse;
  double last_mouse_x;
  double last_mouse_y;
  float camera_yaw;
  float camera_pitch;
  bool keys[1024];

  // Configuration
  int window_width;
  int window_height;
  const char *window_title;
  bool enable_validation;
} engine_state_t;

// Forward declarations - include prefab header for typedef
#include "prefab.h"

// Global configuration
typedef struct
{
  int window_width;
  int window_height;
  const char *window_title;
  bool enable_validation;
  const char *initial_world_path;
  const char *prefabs_directory;
  const char *worlds_directory;
} engine_config_t;

// Default configuration
engine_config_t engine_config_default (void);

// Initialize engine with config
result_t engine_init (engine_state_t *state, const engine_config_t *config);

// Cleanup engine
void engine_cleanup (engine_state_t *state);

// Load world and prefabs (called after engine_init)
result_t engine_load_world (engine_state_t *state,
                            const engine_config_t *config,
                            prefab_system_t **out_prefab_system);

// Main loop
void engine_run (engine_state_t *state);

// GLFW callbacks
void glfw_key_callback (GLFWwindow *window, int key, int scancode, int action,
                        int mods);
void glfw_mouse_callback (GLFWwindow *window, double xpos, double ypos);

// Camera movement (temporary until camera component is implemented)
void process_camera_movement (engine_state_t *state, float delta_time);

#endif // HITE_GLOBAL_H
