#ifndef HITE_SHAPE_COMPONENT_H
#define HITE_SHAPE_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

// SDF shape types
typedef enum
{
  SHAPE_SPHERE = 0,
  SHAPE_BOX = 1,
  SHAPE_TORUS = 2,
  SHAPE_PLANE = 3,
  SHAPE_CYLINDER = 4,
  SHAPE_CAPSULE = 5,
  SHAPE_CONE = 6,
  SHAPE_TERRAIN = 7,
  SHAPE_CUSTOM = 100,
} shape_type_t;

// Shape operations for combining SDFs
typedef enum
{
  SHAPE_OP_UNION = 0,
  SHAPE_OP_SUBTRACTION = 1,
  SHAPE_OP_INTERSECTION = 2,
  SHAPE_OP_SMOOTH_UNION = 3,
} shape_operation_t;

// Custom SDF function pointer (procedural)
typedef float (*sdf_function_t) (vec3_t point, const void *params);

// Shape component data (aligned for GPU transfer)
typedef struct
{
  // Transform
  transform_t transform;

  // Shape definition
  shape_type_t type;
  shape_operation_t operation;

  // Dimensions
  vec3_t dimensions;

  // Material properties
  vec4_t color;
  float roughness;
  float metallic;
  float smoothing; // For smooth operations

  // Custom SDF
  sdf_function_t custom_sdf;
  void *custom_params;
  size_t custom_params_size;

  // Optimization flags
  bool dirty; // Needs GPU update
  bool visible;

  // GPU buffer index
  uint32_t gpu_index;
} ALIGN_64 shape_component_t;

// Transform helpers
vec3_t transform_point (vec3_t point, const transform_t *transform);
vec3_t transform_point_inverse (vec3_t point, const transform_t *transform);

// Evaluate shape component's SDF at a world-space point (CPU stub)
float shape_evaluate_sdf (const shape_component_t *shape, vec3_t world_point);

// Component lifecycle functions
result_t shape_component_start (ecs_world_t *world, entity_id_t entity,
                                void *component_data);
result_t shape_component_update (ecs_world_t *world, entity_id_t entity,
                                 void *component_data,
                                 const time_info_t *time);
result_t shape_component_render (ecs_world_t *world, entity_id_t entity,
                                 const void *component_data);
void shape_component_destroy (void *component_data);

// Component descriptor registration
void shape_component_register (ecs_world_t *world);

// Helper constructors for common shapes
shape_component_t shape_sphere_create (vec3_t position, float radius,
                                       vec4_t color);
shape_component_t shape_box_create (vec3_t position, vec3_t half_extents,
                                    vec4_t color);
shape_component_t shape_torus_create (vec3_t position, float major_radius,
                                      float minor_radius, vec4_t color);

#endif // HITE_SHAPE_COMPONENT_H
