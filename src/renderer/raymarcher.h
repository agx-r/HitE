#ifndef HITE_RAYMARCHER_H
#define HITE_RAYMARCHER_H

#include "../core/ecs.h"
#include "vulkan_core.h"

#define MAX_SDF_OBJECTS 256

typedef struct
{
  mat4_t view_matrix;
  mat4_t projection_matrix;
  vec4_t camera_position;
  vec4_t camera_direction;
  vec2_t resolution;
  vec4_t background_color;
  float time;
  uint32_t object_count;

  float _padding[1];
} ALIGN_64 raymarch_uniforms_t;

typedef struct
{
  vec4_t position;
  vec4_t color;
  vec4_t dimensions;

  vec4_t params;

} ALIGN_64 sdf_object_t;

typedef struct
{
  vulkan_context_t *vk_context;

  VkPipeline compute_pipeline;
  VkPipelineLayout pipeline_layout;
  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_set;

  VkShaderModule compute_shader;

  gpu_buffer_t uniform_buffer;
  gpu_buffer_t sdf_objects_buffer;

  gpu_image_t output_color_depth;
  gpu_image_t output_normal;
  gpu_image_t output_final;

  VkPipeline lighting_pipeline;
  VkPipelineLayout lighting_pipeline_layout;
  VkDescriptorSetLayout lighting_descriptor_set_layout;
  VkDescriptorSet lighting_descriptor_set;
  VkShaderModule lighting_shader;

  gpu_buffer_t lighting_uniform_buffer;

  VkCommandBuffer compute_command_buffer;
  VkFence compute_fence;

  uint32_t width;
  uint32_t height;
  uint32_t max_objects;
} raymarcher_t;

result_t raymarcher_create (vulkan_context_t *context, uint32_t width,
                            uint32_t height, raymarcher_t *raymarcher);
void raymarcher_destroy (raymarcher_t *raymarcher);

result_t raymarcher_load_shader (raymarcher_t *raymarcher,
                                 const char *shader_path);

result_t raymarcher_update_from_ecs (raymarcher_t *raymarcher,
                                     ecs_world_t *world);

result_t raymarcher_execute (raymarcher_t *raymarcher,
                             const raymarch_uniforms_t *uniforms);

typedef struct
{
  vec4_t sun_direction;
  vec4_t sun_color;
  vec4_t camera_position;
  vec4_t camera_direction;
  vec2_t resolution;
  vec4_t background_color;
  float ambient_strength;
  float diffuse_strength;
  float shadow_bias;
  float shadow_softness;
  uint32_t shadow_steps;
  uint32_t object_count;
} ALIGN_64 lighting_uniforms_t;

result_t raymarcher_load_lighting_shader (raymarcher_t *raymarcher,
                                          const char *shader_path);
result_t raymarcher_execute_lighting (raymarcher_t *raymarcher,
                                      const lighting_uniforms_t *uniforms);

const gpu_image_t *raymarcher_get_color_depth (const raymarcher_t *raymarcher);
const gpu_image_t *raymarcher_get_normal (const raymarcher_t *raymarcher);
const gpu_image_t *raymarcher_get_final (const raymarcher_t *raymarcher);

#endif
