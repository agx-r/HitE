#ifndef HITE_SHAPE_COMPONENT_H
#define HITE_SHAPE_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

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
  SHAPE_CITADEL = 8,
  SHAPE_CUSTOM = 256,
} shape_type_t;

typedef enum
{
  SHAPE_OP_UNION = 0,
  SHAPE_OP_SUBTRACTION = 1,
  SHAPE_OP_INTERSECTION = 2,
  SHAPE_OP_SMOOTH_UNION = 3,
} shape_operation_t;

typedef float (*sdf_function_t) (vec3_t point, const void *params);

typedef struct
{

  transform_t transform;

  shape_type_t type;
  shape_operation_t operation;

  vec3_t dimensions;
  float size;

  vec4_t color;
  float roughness;
  float metallic;
  float smoothing;

  sdf_function_t custom_sdf;
  void *custom_params;
  size_t custom_params_size;

  bool dirty;
  bool visible;

  uint32_t gpu_index;
} ALIGN_64 shape_component_t;

vec3_t shape_transform_point (vec3_t point, const transform_t *transform);
vec3_t shape_transform_point_inverse (vec3_t point,
                                      const transform_t *transform);

float shape_evaluate_sdf (const shape_component_t *shape, vec3_t world_point,
                          float time_seconds);

result_t shape_component_start (ecs_world_t *world, entity_id_t entity,
                                void *component_data);
result_t shape_component_update (ecs_world_t *world, entity_id_t entity,
                                 void *component_data,
                                 const time_info_t *time);
result_t shape_component_render (ecs_world_t *world, entity_id_t entity,
                                 const void *component_data);
void shape_component_destroy (void *component_data);

void shape_component_register (ecs_world_t *world);

shape_component_t shape_sphere_create (vec3_t position, float radius,
                                       vec4_t color);
shape_component_t shape_box_create (vec3_t position, vec3_t half_extents,
                                    vec4_t color);
shape_component_t shape_torus_create (vec3_t position, float major_radius,
                                      float minor_radius, vec4_t color);

#endif
