#include "render_system.h"
#include "../components/camera_component.h"
#include "../components/lighting_component.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void
shape_to_sdf_object (const shape_component_t *shape, sdf_object_t *sdf)
{
  sdf->position
      = (vec4_t){ shape->transform.position.x, shape->transform.position.y,
                  shape->transform.position.z, shape->dimensions.x };

  sdf->color = shape->color;

  sdf->dimensions = (vec4_t){ shape->dimensions.x, shape->dimensions.y,
                              shape->dimensions.z, (float)shape->type };

  sdf->params = (vec4_t){ shape->roughness, shape->size, shape->smoothing,
                          shape->metallic };
}

result_t
render_system_init (render_system_t *system, vulkan_context_t *vk_context,
                    GLFWwindow *window, uint32_t width, uint32_t height)
{
  memset (system, 0, sizeof (render_system_t));
  system->window = window;

  result_t result
      = raymarcher_create (vk_context, width, height, &system->raymarcher);
  if (result.code != RESULT_OK)
    return result;

  result = swapchain_create (vk_context, window, width, height,
                             &system->swapchain);
  if (result.code != RESULT_OK)
    {
      raymarcher_destroy (&system->raymarcher);
      return result;
    }

  char exe_path[1024] = { 0 };
  ssize_t len = readlink ("/proc/self/exe", exe_path, sizeof (exe_path) - 1);
  if (len != -1)
    {
      exe_path[len] = '\0';

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
          break;
        }
    }

  if (shader_result.code != RESULT_OK)
    {
      raymarcher_destroy (&system->raymarcher);
      return shader_result;
    }

  char lighting_shader_path_buf[1024] = { 0 };

  if (len != -1 && strlen (exe_path) > 0)
    {
      snprintf (lighting_shader_path_buf, sizeof (lighting_shader_path_buf),
                "%s/share/hite/shaders/lighting.comp.spv", exe_path);
    }
  else
    {
      lighting_shader_path_buf[0] = '\0';
    }

  const char *lighting_shader_paths[]
      = { lighting_shader_path_buf[0] ? lighting_shader_path_buf : NULL,
          "build/shaders/lighting.comp.spv", "shaders/lighting.comp.spv",
          "../shaders/lighting.comp.spv", NULL };

  shader_result = RESULT_ERROR (RESULT_ERROR_FILE_NOT_FOUND,
                                "Lighting shader not found");
  for (int i = 0; lighting_shader_paths[i] != NULL; i++)
    {
      shader_result = raymarcher_load_lighting_shader (
          &system->raymarcher, lighting_shader_paths[i]);
      if (shader_result.code == RESULT_OK)
        {
          break;
        }
    }

  system->sdf_object_capacity = MAX_SDF_OBJECTS;
  system->sdf_objects
      = calloc (system->sdf_object_capacity, sizeof (sdf_object_t));
  if (!system->sdf_objects)
    {
      raymarcher_destroy (&system->raymarcher);
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Failed to allocate SDF object buffer");
    }

  system->camera_position = (vec3_t){ 0, 5, 10, 0 };
  system->camera_direction = (vec3_t){ 0, -0.3f, -1, 0 };

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

  component_id_t shape_id = ecs_get_component_id (world, "shape");
  if (shape_id == INVALID_ENTITY)
    {
      return RESULT_ERROR (RESULT_ERROR_NOT_FOUND,
                           "Shape component not registered");
    }

  system->sdf_object_count = 0;

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

      shape_to_sdf_object (shape,
                           &system->sdf_objects[system->sdf_object_count]);
      system->sdf_object_count++;
    }

  for (size_t i = system->sdf_object_count; i < system->sdf_object_capacity;
       i++)
    {
      memset (&system->sdf_objects[i], 0, sizeof (sdf_object_t));
    }

  result_t result = gpu_buffer_upload (
      system->raymarcher.vk_context, &system->raymarcher.sdf_objects_buffer,
      system->sdf_objects,
      sizeof (sdf_object_t) * system->sdf_object_capacity);

  return result;
}

result_t
render_system_render_frame (render_system_t *system, ecs_world_t *world,
                            float time)
{
  if (!system)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid system");
    }

  raymarch_uniforms_t uniforms = { 0 };

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

  entity_id_t camera_entity_for_bg = INVALID_ENTITY;
  camera_component_t *camera_for_bg = NULL;
  if (world)
    {
      camera_for_bg = camera_find_active (world, &camera_entity_for_bg);
    }
  if (camera_for_bg)
    {
      uniforms.background_color
          = (vec4_t){ camera_for_bg->background_color.x,
                      camera_for_bg->background_color.y,
                      camera_for_bg->background_color.z, 0 };
    }
  else
    {
      uniforms.background_color = (vec4_t){ 0.01f, 0.01f, 0.01f, 0 };
    }

  uniforms.time = time;

  uniforms.object_count = (system->sdf_object_count > MAX_SDF_OBJECTS)
                              ? MAX_SDF_OBJECTS
                              : system->sdf_object_count;

  result_t result = raymarcher_execute (&system->raymarcher, &uniforms);
  if (result.code != RESULT_OK)
    return result;

  entity_id_t camera_entity = INVALID_ENTITY;
  camera_component_t *camera = NULL;
  lighting_component_t *lighting = NULL;

  if (world)
    {
      camera = camera_find_active (world, &camera_entity);
      if (camera_entity != INVALID_ENTITY)
        {
          lighting = lighting_find_on_camera (world, camera_entity);
        }
    }

  if (lighting && lighting->enabled && system->raymarcher.lighting_pipeline)
    {
      lighting_uniforms_t lighting_uniforms = { 0 };

      lighting_uniforms.sun_direction
          = (vec4_t){ lighting->sun_direction.x, lighting->sun_direction.y,
                      lighting->sun_direction.z, 0 };
      lighting_uniforms.sun_color
          = (vec4_t){ lighting->sun_color.x, lighting->sun_color.y,
                      lighting->sun_color.z, 0 };
      lighting_uniforms.camera_position
          = (vec4_t){ system->camera_position.x, system->camera_position.y,
                      system->camera_position.z, 0 };
      lighting_uniforms.camera_direction
          = (vec4_t){ system->camera_direction.x, system->camera_direction.y,
                      system->camera_direction.z, 0 };
      lighting_uniforms.resolution
          = (vec2_t){ (float)system->raymarcher.width,
                      (float)system->raymarcher.height, 0, 0 };
      if (camera)
        {
          lighting_uniforms.background_color
              = (vec4_t){ camera->background_color.x,
                          camera->background_color.y,
                          camera->background_color.z, 0 };
        }
      else
        {
          lighting_uniforms.background_color
              = (vec4_t){ 0.06f, 0.06f, 0.06f, 0 };
        }
      lighting_uniforms.time = time;
      lighting_uniforms.ambient_strength = lighting->ambient_strength;
      lighting_uniforms.diffuse_strength = lighting->diffuse_strength;
      lighting_uniforms.shadow_bias = lighting->shadow_bias;
      lighting_uniforms.shadow_softness = lighting->shadow_softness;
      lighting_uniforms.shadow_steps = (uint32_t)lighting->shadow_steps;
      lighting_uniforms.object_count
          = (system->sdf_object_count > MAX_SDF_OBJECTS)
                ? MAX_SDF_OBJECTS
                : system->sdf_object_count;

      result = raymarcher_execute_lighting (&system->raymarcher,
                                            &lighting_uniforms);
      if (result.code != RESULT_OK)
        return result;

      return swapchain_present (system->raymarcher.vk_context,
                                &system->swapchain,
                                raymarcher_get_final (&system->raymarcher));
    }
  else
    {

      return swapchain_present (
          system->raymarcher.vk_context, &system->swapchain,
          raymarcher_get_color_depth (&system->raymarcher));
    }
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
