#ifndef HITE_GLOBAL_H
#define HITE_GLOBAL_H

#include "../renderer/render_system.h"
#include "../renderer/vulkan_core.h"
#include "events.h"
#include "resources.h"
#include "types.h"
#include "world.h"

#include <GLFW/glfw3.h>
#include <stdbool.h>

typedef struct
{

  GLFWwindow *window;
  vulkan_context_t vk_context;
  render_system_t render_system;
  world_manager_t *world_manager;
  event_system_t *event_system;
  resource_manager_t *resource_manager;

  bool running;
  double last_time;

  bool first_mouse;
  double last_mouse_x;
  double last_mouse_y;
  float camera_yaw;
  float camera_pitch;
  bool keys[1024];

  int window_width;
  int window_height;
  const char *window_title;
  bool enable_validation;
} engine_state_t;

#include "prefab.h"

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

engine_config_t engine_config_default (void);

result_t engine_init (engine_state_t *state, const engine_config_t *config);

void engine_cleanup (engine_state_t *state);

result_t engine_load_world (engine_state_t *state,
                            const engine_config_t *config,
                            prefab_system_t **out_prefab_system);

void engine_run (engine_state_t *state);

void glfw_key_callback (GLFWwindow *window, int key, int scancode, int action,
                        int mods);
void glfw_mouse_callback (GLFWwindow *window, double xpos, double ypos);

void process_camera_movement (engine_state_t *state, float delta_time);

#endif
