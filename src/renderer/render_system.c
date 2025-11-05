#include "render_system.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Convert shape component to GPU SDF object
static void
shape_to_sdf_object (const shape_component_t *shape, sdf_object_t *sdf)
{
  sdf->position
      = (vec4_t){ shape->transform.position.x, shape->transform.position.y,
                  shape->transform.position.z,
                  shape->dimensions.x }; // w = radius/scale

  sdf->color = shape->color;

  sdf->dimensions
      = (vec4_t){ shape->dimensions.x, shape->dimensions.y,
                  shape->dimensions.z,
                  (float)shape->type }; // w = type (stored as float)

  sdf->params = (vec4_t){ shape->smoothing, // x = smoothing
                          0.0f,             // y = material_id
                          0.0f, 0.0f };     // z,w = reserved
}

result_t
render_system_init (render_system_t *system, vulkan_context_t *vk_context,
                    GLFWwindow *window, uint32_t width, uint32_t height)
{
  memset (system, 0, sizeof (render_system_t));
  system->window = window;

  // Init raymarcher
  result_t result
      = raymarcher_create (vk_context, width, height, &system->raymarcher);
  if (result.code != RESULT_OK)
    return result;

  // Initialize swapchain
  result = swapchain_create (vk_context, window, width, height,
                             &system->swapchain);
  if (result.code != RESULT_OK)
    {
      raymarcher_destroy (&system->raymarcher);
      return result;
    }

  // Get executable path for relative shader location
  char exe_path[1024];
  ssize_t len = readlink ("/proc/self/exe", exe_path, sizeof (exe_path) - 1);
  if (len != -1)
    {
      exe_path[len] = '\0';
      // Remove "/bin/hite" to get install prefix
      char *last_slash = strrchr (exe_path, '/');
      if (last_slash)
        {
          *last_slash = '\0';
          last_slash = strrchr (exe_path, '/');
          if (last_slash)
            *last_slash = '\0';
        }
    }

  char shader_path_buf[1024];
  snprintf (shader_path_buf, sizeof (shader_path_buf),
            "%s/share/hite/shaders/raymarch.comp.spv", exe_path);

  const char *shader_paths[]
      = { shader_path_buf, "build/shaders/raymarch.comp.spv",
          "shaders/raymarch.comp.spv", "../shaders/raymarch.comp.spv", NULL };

  result_t shader_result
      = RESULT_ERROR (RESULT_ERROR_FILE_NOT_FOUND, "Shader not found");
  for (int i = 0; shader_paths[i] != NULL; i++)
    {
      shader_result
          = raymarcher_load_shader (&system->raymarcher, shader_paths[i]);
      if (shader_result.code == RESULT_OK)
        {
          printf ("[Render] Loaded shader: %s\n", shader_paths[i]);
          break;
        }
    }

  if (shader_result.code != RESULT_OK)
    {
      raymarcher_destroy (&system->raymarcher);
      return shader_result;
    }

  // Allocate SDF object buffer
  system->sdf_object_capacity = MAX_SDF_OBJECTS;
  system->sdf_objects
      = calloc (system->sdf_object_capacity, sizeof (sdf_object_t));
  if (!system->sdf_objects)
    {
      raymarcher_destroy (&system->raymarcher);
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Failed to allocate SDF object buffer");
    }

  // Default camera - looking at the scene from above and behind
  system->camera_position = (vec3_t){ 0, 5, 10, 0 };
  system->camera_direction = (vec3_t){ 0, -0.3f, -1, 0 };

  // Normalize direction
  float dir_len
      = sqrtf (system->camera_direction.x * system->camera_direction.x
               + system->camera_direction.y * system->camera_direction.y
               + system->camera_direction.z * system->camera_direction.z);
  system->camera_direction.x /= dir_len;
  system->camera_direction.y /= dir_len;
  system->camera_direction.z /= dir_len;

  system->camera_fov = 1.0f;

  return RESULT_SUCCESS;
}

void
render_system_cleanup (render_system_t *system)
{
  if (!system)
    return;

  swapchain_destroy (system->raymarcher.vk_context, &system->swapchain);
  free (system->sdf_objects);
  raymarcher_destroy (&system->raymarcher);
}

result_t
render_system_collect_shapes (render_system_t *system, ecs_world_t *world)
{
  if (!system || !world)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

  // Find shape component ID
  component_id_t shape_id = ecs_get_component_id (world, "shape");
  if (shape_id == INVALID_ENTITY)
    {
      return RESULT_ERROR (RESULT_ERROR_NOT_FOUND,
                           "Shape component not registered");
    }

  // Reset counter
  system->sdf_object_count = 0;

  // Iterate through all shape components
  component_array_t *shape_array = &world->component_arrays[shape_id];

  for (size_t i = 0; i < shape_array->count && i < system->sdf_object_capacity;
       i++)
    {
      if (!shape_array->active[i])
        {
          continue;
        }

      const shape_component_t *shape
          = (const shape_component_t
                 *)((char *)shape_array->data
                    + i * shape_array->descriptor.data_size);

      if (!shape->visible)
        {
          continue;
        }

      // Convert to GPU format
      shape_to_sdf_object (shape,
                           &system->sdf_objects[system->sdf_object_count]);
      system->sdf_object_count++;
    }

  // Clear remaining slots to mark end of object list
  for (size_t i = system->sdf_object_count; i < system->sdf_object_capacity;
       i++)
    {
      memset (&system->sdf_objects[i], 0, sizeof (sdf_object_t));
    }

  // Upload entire buffer to GPU (including cleared slots for termination)
  result_t result = gpu_buffer_upload (
      system->raymarcher.vk_context, &system->raymarcher.sdf_objects_buffer,
      system->sdf_objects,
      sizeof (sdf_object_t) * system->sdf_object_capacity);

  return result;
}

result_t
render_system_render_frame (render_system_t *system, float time)
{
  if (!system)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid system");
    }

  // Setup uniforms
  raymarch_uniforms_t uniforms = { 0 };

  // Identity matrices (simplified - should use proper math library)
  for (int i = 0; i < 4; i++)
    {
      uniforms.view_matrix.rows[i].x = (i == 0) ? 1.0f : 0.0f;
      uniforms.view_matrix.rows[i].y = (i == 1) ? 1.0f : 0.0f;
      uniforms.view_matrix.rows[i].z = (i == 2) ? 1.0f : 0.0f;
      uniforms.view_matrix.rows[i].w = (i == 3) ? 1.0f : 0.0f;

      uniforms.projection_matrix.rows[i] = uniforms.view_matrix.rows[i];
    }

  uniforms.camera_position
      = (vec4_t){ system->camera_position.x, system->camera_position.y,
                  system->camera_position.z, 0 };

  uniforms.camera_direction
      = (vec4_t){ system->camera_direction.x, system->camera_direction.y,
                  system->camera_direction.z, 0 };

  uniforms.resolution = (vec2_t){ (float)system->raymarcher.width,
                                  (float)system->raymarcher.height, 0, 0 };

  uniforms.time = time;
  // Clamp object count to max buffer size for safety
  uniforms.object_count = (system->sdf_object_count > MAX_SDF_OBJECTS) 
                          ? MAX_SDF_OBJECTS 
                          : system->sdf_object_count;

  // Execute raymarching
  result_t result = raymarcher_execute (&system->raymarcher, &uniforms);
  if (result.code != RESULT_OK)
    return result;

  // Present to screen
  return swapchain_present (system->raymarcher.vk_context, &system->swapchain,
                            raymarcher_get_output (&system->raymarcher));
}

void
render_system_set_camera (render_system_t *system, vec3_t position,
                          vec3_t direction)
{
  if (!system)
    return;

  system->camera_position = position;
  system->camera_direction = direction;
}

void
render_system_move_camera (render_system_t *system, vec3_t delta)
{
  if (!system)
    return;

  system->camera_position.x += delta.x;
  system->camera_position.y += delta.y;
  system->camera_position.z += delta.z;
}

void
render_system_rotate_camera (render_system_t *system, float yaw, float pitch)
{
  if (!system)
    return;

  // Simple rotation (should use proper quaternions)
  float cos_yaw = cosf (yaw);
  float sin_yaw = sinf (yaw);
  float cos_pitch = cosf (pitch);
  float sin_pitch = sinf (pitch);

  system->camera_direction.x = cos_pitch * sin_yaw;
  system->camera_direction.y = sin_pitch;
  system->camera_direction.z = cos_pitch * cos_yaw;
}
