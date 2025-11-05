#ifndef HITE_RENDER_SYSTEM_H
#define HITE_RENDER_SYSTEM_H

#include "../components/shape_component.h"
#include "../core/ecs.h"
#include "raymarcher.h"
#include "swapchain.h"
#include <GLFW/glfw3.h>

typedef struct
{
  raymarcher_t raymarcher;
  swapchain_t swapchain;
  GLFWwindow *window;

  vec3_t camera_position;
  vec3_t camera_direction;
  float camera_fov;

  sdf_object_t *sdf_objects;
  size_t sdf_object_count;
  size_t sdf_object_capacity;
} render_system_t;

result_t render_system_init (render_system_t *system,
                             vulkan_context_t *vk_context, GLFWwindow *window,
                             uint32_t width, uint32_t height);
void render_system_cleanup (render_system_t *system);

result_t render_system_collect_shapes (render_system_t *system,
                                       ecs_world_t *world);

result_t render_system_render_frame (render_system_t *system,
                                     ecs_world_t *world, float time);

void render_system_set_camera (render_system_t *system, vec3_t position,
                               vec3_t direction);
void render_system_move_camera (render_system_t *system, vec3_t delta);
void render_system_rotate_camera (render_system_t *system, float yaw,
                                  float pitch);

#endif
