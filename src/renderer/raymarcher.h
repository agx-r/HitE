#ifndef HITE_RAYMARCHER_H
#define HITE_RAYMARCHER_H

#include "../core/ecs.h"
#include "vulkan_core.h"

#define MAX_SDF_OBJECTS 256

// GPU-side data structures (aligned for GPU)
typedef struct
{
  mat4_t view_matrix;       // 64 bytes
  mat4_t projection_matrix; // 64 bytes
  vec4_t camera_position;   // 16 bytes
  vec4_t camera_direction;  // 16 bytes
  vec2_t resolution;        // 16 bytes (with padding)
  float time;               // 4 bytes
  uint32_t object_count;    // 4 bytes - Number of active objects for shader
                            // optimization
  float _padding[2];        // 8 bytes - Align to 192 bytes (3?64)
} ALIGN_64 raymarch_uniforms_t;

// Note: SDF object types are defined in shape_component.h as shape_type_t
// We don't duplicate them here to avoid inconsistencies

// SDF object data (GPU-compatible)
// Must be exactly 64 bytes for ALIGN_64 to work correctly
typedef struct
{
  vec4_t position;   // xyz = position, w = radius/scale for sphere
  vec4_t color;      // rgba
  vec4_t dimensions; // xyz = dimensions, w = type (as float, will cast to uint
                     // in shader)
  vec4_t params;     // x = smoothing, y = material_id, z/w = reserved
  // 64 bytes total (4 * vec4 = 4 * 16 = 64) - perfect alignment
} ALIGN_64 sdf_object_t;

// Raymarcher state
typedef struct
{
  vulkan_context_t *vk_context;

  // Compute pipeline
  VkPipeline compute_pipeline;
  VkPipelineLayout pipeline_layout;
  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_set;

  // Shader module
  VkShaderModule compute_shader;

  // GPU buffers
  gpu_buffer_t uniform_buffer;
  gpu_buffer_t sdf_objects_buffer;

  // Output image
  gpu_image_t output_image;

  // Compute command buffers
  VkCommandBuffer compute_command_buffer;
  VkFence compute_fence;

  // Configuration
  uint32_t width;
  uint32_t height;
  uint32_t max_objects;
} raymarcher_t;

// Raymarcher lifecycle
result_t raymarcher_create (vulkan_context_t *context, uint32_t width,
                            uint32_t height, raymarcher_t *raymarcher);
void raymarcher_destroy (raymarcher_t *raymarcher);

// Shader management
result_t raymarcher_load_shader (raymarcher_t *raymarcher,
                                 const char *shader_path);

// Update GPU buffers from ECS data
result_t raymarcher_update_from_ecs (raymarcher_t *raymarcher,
                                     ecs_world_t *world);

// Execute raymarching
result_t raymarcher_execute (raymarcher_t *raymarcher,
                             const raymarch_uniforms_t *uniforms);

// Get output image for display
const gpu_image_t *raymarcher_get_output (const raymarcher_t *raymarcher);

#endif // HITE_RAYMARCHER_H
