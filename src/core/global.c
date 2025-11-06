#include "global.h"
#include "../components/camera_component.h"
#include "../components/camera_movement_component.h"
#include "../components/camera_rotation_component.h"
#include "../components/component_registry.h"
#include "../components/developer_overlay_component.h"
#include "../components/lighting_component.h"
#include "../components/shape_component.h"
#include "component_parsers.h"
#include "prefab.h"
#include "scheme_parser.h"
#include "world_loader.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_WINDOW_WIDTH 1280
#define DEFAULT_WINDOW_HEIGHT 720
#define DEFAULT_WINDOW_TITLE "HitE"

engine_config_t
engine_config_default (void)
{
  engine_config_t config = { 0 };
  config.window_width = DEFAULT_WINDOW_WIDTH;
  config.window_height = DEFAULT_WINDOW_HEIGHT;
  config.window_title = DEFAULT_WINDOW_TITLE;
  config.enable_validation = true;
  config.initial_world_path = "worlds/example.scm";
  config.prefabs_directory = "prefabs";
  config.worlds_directory = "worlds";
  return config;
}

void
glfw_key_callback (GLFWwindow *window, int key, int scancode, int action,
                   int mods)
{
  (void)scancode;

  engine_state_t *state = (engine_state_t *)glfwGetWindowUserPointer (window);

  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
      state->running = false;
    }

  if (key >= 0 && key < 1024)
    {
      if (action == GLFW_PRESS)
        {
          state->keys[key] = true;
        }
      else if (action == GLFW_RELEASE)
        {
          state->keys[key] = false;
        }
    }

  if (state->event_system)
    {
      event_type_t type
          = (action == GLFW_PRESS) ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE;
      event_t event = event_key_create (type, key, mods);
      event_emit (state->event_system, &event);
    }
}

void
glfw_mouse_callback (GLFWwindow *window, double xpos, double ypos)
{
  engine_state_t *state = (engine_state_t *)glfwGetWindowUserPointer (window);

  if (state->first_mouse)
    {
      state->last_mouse_x = xpos;
      state->last_mouse_y = ypos;
      state->first_mouse = false;
    }

  double xoffset = xpos - state->last_mouse_x;
  double yoffset = ypos - state->last_mouse_y;
  state->last_mouse_x = xpos;
  state->last_mouse_y = ypos;

  float sensitivity = 0.003f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  state->camera_yaw -= xoffset;
  state->camera_pitch -= yoffset;

  if (state->camera_pitch > 1.5f)
    state->camera_pitch = 1.5f;
  if (state->camera_pitch < -1.5f)
    state->camera_pitch = -1.5f;

  render_system_rotate_camera (&state->render_system, state->camera_yaw,
                               state->camera_pitch);
}

void
process_camera_movement (engine_state_t *state, float delta_time)
{
  float camera_speed = 5.0f * delta_time;

  vec3_t forward = state->render_system.camera_direction;
  vec3_t right;

  right.x = -forward.z;
  right.y = 0;
  right.z = forward.x;

  float len = sqrtf (right.x * right.x + right.z * right.z);
  if (len > 0.0001f)
    {
      right.x /= len;
      right.z /= len;
    }

  vec3_t movement = { 0, 0, 0, 0 };

  if (state->keys[GLFW_KEY_W])
    {
      movement.x += forward.x * camera_speed;
      movement.y += forward.y * camera_speed;
      movement.z += forward.z * camera_speed;
    }
  if (state->keys[GLFW_KEY_S])
    {
      movement.x -= forward.x * camera_speed;
      movement.y -= forward.y * camera_speed;
      movement.z -= forward.z * camera_speed;
    }
  if (state->keys[GLFW_KEY_A])
    {
      movement.x -= right.x * camera_speed;
      movement.z -= right.z * camera_speed;
    }
  if (state->keys[GLFW_KEY_D])
    {
      movement.x += right.x * camera_speed;
      movement.z += right.z * camera_speed;
    }
  if (state->keys[GLFW_KEY_SPACE])
    {
      movement.y += camera_speed;
    }
  if (state->keys[GLFW_KEY_LEFT_SHIFT])
    {
      movement.y -= camera_speed;
    }

  render_system_move_camera (&state->render_system, movement);
}

result_t
engine_init (engine_state_t *state, const engine_config_t *config)
{
  memset (state, 0, sizeof (engine_state_t));
  state->running = true;
  state->window_width = config->window_width;
  state->window_height = config->window_height;
  state->window_title = config->window_title;
  state->enable_validation = config->enable_validation;

  if (!glfwInit ())
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to initialize GLFW");
    }

  glfwInitHint (GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);

  glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);

  state->window = glfwCreateWindow (state->window_width, state->window_height,
                                    state->window_title, NULL, NULL);
  if (!state->window)
    {
      glfwTerminate ();
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to create window");
    }

  glfwSetWindowUserPointer (state->window, state);
  glfwSetKeyCallback (state->window, glfw_key_callback);
  glfwSetCursorPosCallback (state->window, glfw_mouse_callback);

  glfwSetInputMode (state->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  state->first_mouse = true;
  state->camera_yaw = -M_PI;
  state->camera_pitch = -0.3f;
  memset (state->keys, 0, sizeof (state->keys));

  result_t result = vulkan_init (&state->vk_context, state->enable_validation);
  if (result.code != RESULT_OK)
    return result;

  state->world_manager = world_manager_create ();
  state->event_system = event_system_create ();
  state->resource_manager = resource_manager_create ();

  if (!state->world_manager || !state->event_system
      || !state->resource_manager)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Failed to create subsystems");
    }

  result = render_system_init (&state->render_system, &state->vk_context,
                               state->window, state->window_width,
                               state->window_height);
  if (result.code != RESULT_OK)
    return result;

  printf ("[Engine] Initialized successfully\n");

  return RESULT_SUCCESS;
}

void
engine_cleanup (engine_state_t *state)
{
  if (!state)
    return;

  render_system_cleanup (&state->render_system);
  world_manager_destroy (state->world_manager);
  event_system_destroy (state->event_system);
  resource_manager_destroy (state->resource_manager);
  vulkan_cleanup (&state->vk_context);

  if (state->window)
    {
      glfwDestroyWindow (state->window);
    }
  glfwTerminate ();

  printf ("[Engine] Cleaned up\n");
}

result_t
engine_load_world (engine_state_t *state, const engine_config_t *config,
                   prefab_system_t **out_prefab_system)
{
  if (!state || !config)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid arguments");
    }

  prefab_system_t *prefab_system = prefab_system_create ();
  if (!prefab_system)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Failed to create prefab system");
    }

  prefab_system_set_directory (prefab_system, config->prefabs_directory);

  state->world_manager->active_world = ecs_world_create ();
  if (!state->world_manager->active_world)
    {
      prefab_system_destroy (prefab_system);
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Failed to create ECS world");
    }

  printf ("[Engine] Registering components...\n");
  camera_component_register (state->world_manager->active_world);
  camera_movement_component_register (state->world_manager->active_world);
  camera_rotation_component_register (state->world_manager->active_world);
  lighting_component_register (state->world_manager->active_world);
  shape_component_register (state->world_manager->active_world);
  developer_overlay_component_register (state->world_manager->active_world);
  printf ("[Engine] Components registered\n");

  world_definition_t world_def = { 0 };
  result_t result
      = world_load_from_file (config->initial_world_path, &world_def);
  if (result.code != RESULT_OK)
    {
      printf ("[Warning] Failed to load world file: %s\n", result.message);
      world_def.name = "Default World";
      world_def.fixed_delta_time = 1.0f / 60.0f;
      world_def.use_fixed_timestep = false;
    }

  result = world_load (state->world_manager, &world_def, prefab_system);
  if (result.code != RESULT_OK)
    {
      world_definition_free (&world_def);
      prefab_system_destroy (prefab_system);
      return result;
    }

  world_definition_free (&world_def);

  if (out_prefab_system)
    *out_prefab_system = prefab_system;
  else
    prefab_system_destroy (prefab_system);

  printf ("[Engine] World loaded successfully\n");

  return RESULT_SUCCESS;
}

static void
update_camera_from_component (engine_state_t *state)
{
  if (!state->world_manager || !state->world_manager->active_world)
    return;

  camera_component_t *camera
      = camera_find_active (state->world_manager->active_world, NULL);
  if (camera)
    {

      render_system_set_camera (&state->render_system, camera->position,
                                camera->direction);
    }
}

void
engine_run (engine_state_t *state)
{
  state->last_time = glfwGetTime ();

  printf ("\n[Engine] Starting main loop...\n");
  printf ("Controls:\n");
  printf ("  WASD - Move camera\n");
  printf ("  Mouse - Look around\n");
  printf ("  Space - Move up\n");
  printf ("  Shift - Move down\n");
  printf ("  ESC - Exit\n\n");

  while (state->running && !glfwWindowShouldClose (state->window))
    {
      double current_time = glfwGetTime ();
      float delta_time = (float)(current_time - state->last_time);
      state->last_time = current_time;

      glfwPollEvents ();
      event_process (state->event_system);

      if (state->world_manager->active_world)
        {
          world_update (state->world_manager, delta_time);

          update_camera_from_component (state);

          render_system_collect_shapes (&state->render_system,
                                        state->world_manager->active_world);
        }

      render_system_render_frame (&state->render_system,
                                  state->world_manager->active_world,
                                  (float)current_time);

      static double last_reload_check = 0;
      if (current_time - last_reload_check > 1.0)
        {
          resource_check_reload (state->resource_manager);
          last_reload_check = current_time;
        }
    }
}
