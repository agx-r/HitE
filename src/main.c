#include "components/physics_component.h"
#include "components/shape_component.h"
#include "core/ecs.h"
#include "core/events.h"
#include "core/resources.h"
#include "core/types.h"
#include "core/world.h"
#include "renderer/render_system.h"
#include "renderer/vulkan_core.h"

#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

// Global state
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

  // Cam control
  bool first_mouse;
  double last_mouse_x;
  double last_mouse_y;
  float camera_yaw;
  float camera_pitch;
  bool keys[1024];
} engine_state_t;

// GLFW callbacks
static void
key_callback (GLFWwindow *window, int key, int scancode, int action, int mods)
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

static void
mouse_callback (GLFWwindow *window, double xpos, double ypos)
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

static void
process_camera_movement (engine_state_t *state, float delta_time)
{
  float camera_speed = 5.0f * delta_time;

  vec3_t forward = state->render_system.camera_direction;
  vec3_t right;

  right.x = -forward.z;
  right.y = 0;
  right.z = forward.x;

  // Normalize
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

// Example world generator
static result_t
example_world_generator (ecs_world_t *world, void *user_data)
{
  (void)user_data;

  printf ("[World] Generating procedural scene...\n");

  component_id_t shape_id = ecs_get_component_id (world, "shape");
  if (shape_id == INVALID_ENTITY)
    {
      printf ("[World] ERROR: Shape component not found!\n");
      return RESULT_ERROR (RESULT_ERROR_NOT_FOUND,
                           "Shape component not registered");
    }

  // Big test sphere right in front of camera
  entity_id_t test_sphere = ecs_entity_create (world);
  shape_component_t test_shape = shape_sphere_create (
      (vec3_t){ 0, 3, 0, 0 },
      1.5f, (vec4_t){ 1.0f, 0.64f, 0.83f, 1.0f }
  );
  result_t result
      = ecs_add_component (world, test_sphere, shape_id, &test_shape);
  printf ("[World] Added test sphere: %s\n",
          result.code == RESULT_OK ? "OK" : result.message);

  // Create ground plane
  entity_id_t ground = ecs_entity_create (world);
  shape_component_t ground_shape
      = shape_box_create ((vec3_t){ 0, -1, 0, 0 }, (vec3_t){ 20, 0.1f, 20, 0 },
                          (vec4_t){ 0.3f, 0.5f, 0.3f, 1.0f });
  result = ecs_add_component (world, ground, shape_id, &ground_shape);
  printf ("[World] Added ground: %s\n",
          result.code == RESULT_OK ? "OK" : result.message);

  // Create some spheres
  for (int i = 0; i < 5; i++)
    {
      entity_id_t sphere = ecs_entity_create (world);

      float x = (float)(i - 2) * 2.0f;
      float y = 2.0f;
      float radius = 0.6f;

      vec4_t color = { 0.2f + (float)i * 0.15f, 0.3f + (float)(4 - i) * 0.15f,
                       0.7f, 1.0f };

      shape_component_t sphere_shape
          = shape_sphere_create ((vec3_t){ x, y, 0, 0 }, radius, color);

      result = ecs_add_component (world, sphere, shape_id, &sphere_shape);
      printf ("[World] Added sphere %d at (%.1f, %.1f, 0): %s\n", i, x, y,
              result.code == RESULT_OK ? "OK" : result.message);
    }

  printf ("[World] Generated 7 entities\n");

  return RESULT_SUCCESS;
}

// Initialize engine
static result_t
engine_init (engine_state_t *state)
{
  memset (state, 0, sizeof (engine_state_t));
  state->running = true;

  // Initialize GLFW
  if (!glfwInit ())
    {
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to initialize GLFW");
    }

  // Force Wayland only
  glfwInitHint (GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);

  glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);

  state->window = glfwCreateWindow (WINDOW_WIDTH, WINDOW_HEIGHT, "Hite Engine",
                                    NULL, NULL);
  if (!state->window)
    {
      glfwTerminate ();
      return RESULT_ERROR (RESULT_ERROR_VULKAN, "Failed to create window");
    }

  glfwSetWindowUserPointer (state->window, state);
  glfwSetKeyCallback (state->window, key_callback);
  glfwSetCursorPosCallback (state->window, mouse_callback);

  // Capture mouse
  glfwSetInputMode (state->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // Initialize camera state to match initial direction
  state->first_mouse = true;
  state->camera_yaw = -M_PI;
  state->camera_pitch = -0.3f;
  memset (state->keys, 0, sizeof (state->keys));

  // Initialize Vulkan
  result_t result = vulkan_init (&state->vk_context, true);
  if (result.code != RESULT_OK)
    return result;

  // Initialize subsystems
  state->world_manager = world_manager_create ();
  state->event_system = event_system_create ();
  state->resource_manager = resource_manager_create ();

  if (!state->world_manager || !state->event_system
      || !state->resource_manager)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Failed to create subsystems");
    }

  // Initialize render system
  result = render_system_init (&state->render_system, &state->vk_context,
                               state->window, WINDOW_WIDTH, WINDOW_HEIGHT);
  if (result.code != RESULT_OK)
    return result;

  printf ("[Engine] Initialized successfully\n");

  return RESULT_SUCCESS;
}

// Cleanup engine
static void
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

// Main loop
static void
engine_run (engine_state_t *state)
{
  state->last_time = glfwGetTime ();

  while (state->running && !glfwWindowShouldClose (state->window))
    {
      double current_time = glfwGetTime ();
      float delta_time = (float)(current_time - state->last_time);
      state->last_time = current_time;

      // Poll events
      glfwPollEvents ();
      event_process (state->event_system);

      // Camera movement TODO: should be component
      process_camera_movement (state, delta_time);

      // Update world
      if (state->world_manager->active_world)
        {
          world_update (state->world_manager, delta_time);

          // Collect shapes for rendering
          render_system_collect_shapes (&state->render_system,
                                        state->world_manager->active_world);
        }

      // Render
      render_system_render_frame (&state->render_system, (float)current_time);

      // Hot reload check (every ~1 second)
      static double last_reload_check = 0;
      if (current_time - last_reload_check > 1.0)
        {
          resource_check_reload (state->resource_manager);
          last_reload_check = current_time;
        }
    }
}

int
main (int argc, char **argv)
{
  (void)argc;
  (void)argv;

  printf ("=== Hite Engine ===\n");
  printf ("High-performance Vulkan raymarching engine\n\n");

  engine_state_t state;

  // Engine initialize
  result_t result = engine_init (&state);
  if (result.code != RESULT_OK)
    {
      fprintf (stderr, "[Error] Engine initialization failed: %s\n",
               result.message);
      engine_cleanup (&state);
      return 1;
    }

  // Create world and register components
  state.world_manager->active_world = ecs_world_create ();
  if (!state.world_manager->active_world)
    {
      fprintf (stderr, "[Error] Failed to create ECS world\n");
      engine_cleanup (&state);
      return 1;
    }

  printf ("[Engine] Registering components...\n");
  shape_component_register (state.world_manager->active_world);
  physics_component_register (state.world_manager->active_world);
  printf ("[Engine] Components registered\n");

  world_definition_t example_world = { 0 };
  example_world.name = "Example World";
  example_world.fixed_delta_time = 1.0f / 60.0f;
  example_world.use_fixed_timestep = false;
  example_world.generator = example_world_generator;

  result = world_load (state.world_manager, &example_world);
  if (result.code != RESULT_OK)
    {
      fprintf (stderr, "[Error] Failed to load world: %s\n", result.message);
      engine_cleanup (&state);
      return 1;
    }

  printf ("\n[Engine] Starting main loop...\n");
  printf ("Controls:\n");
  printf ("  WASD - Move camera\n");
  printf ("  Mouse - Look around\n");
  printf ("  Space - Move up\n");
  printf ("  Shift - Move down\n");
  printf ("  ESC - Exit\n\n");

  engine_run (&state);

  engine_cleanup (&state);

  printf ("\n[Engine] Bye\n");
  return 0;
}
