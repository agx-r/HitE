#include "shape_component.h"
#include "component_registry.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Legacy CPU SDF includes removed. GPU is authoritative for rendering.

// Transform helpers (simplified - full implementation would use quaternions)
vec3_t
transform_point (vec3_t point, const transform_t *transform)
{
  return (vec3_t){ .x = point.x + transform->position.x,
                   .y = point.y + transform->position.y,
                   .z = point.z + transform->position.z,
                   ._padding = 0 };
}

vec3_t
transform_point_inverse (vec3_t point, const transform_t *transform)
{
  return (vec3_t){ .x = point.x - transform->position.x,
                   .y = point.y - transform->position.y,
                   .z = point.z - transform->position.z,
                   ._padding = 0 };
}

// CPU-side SDF evaluation no longer used for rendering; keep safe stub.
float
shape_evaluate_sdf (const shape_component_t *shape, vec3_t world_point)
{
  (void)shape;
  (void)world_point;
  return 1000000.0f;
}

// Component lifecycle
result_t
shape_component_start (ecs_world_t *world, entity_id_t entity,
                       void *component_data)
{
  (void)world;
  (void)entity;

  shape_component_t *shape = (shape_component_t *)component_data;

  if (shape->color.w == 0.0f)
    {
      shape->color = (vec4_t){ .x = 0.8f, .y = 0.8f, .z = 0.8f, .w = 1.0f };
    }

  shape->dirty = true;
  shape->visible = true;

  return RESULT_SUCCESS;
}

result_t
shape_component_update (ecs_world_t *world, entity_id_t entity,
                        void *component_data, const time_info_t *time)
{
  (void)world;
  (void)entity;
  (void)time;
  (void)component_data;
  return RESULT_SUCCESS;
}

result_t
shape_component_render (ecs_world_t *world, entity_id_t entity,
                        const void *component_data)
{
  (void)world;
  (void)entity;
  (void)component_data;
  return RESULT_SUCCESS;
}

void
shape_component_destroy (void *component_data)
{
  (void)component_data;
}

void
shape_component_register (ecs_world_t *world)
{
  REGISTER_COMPONENT (world, "shape", shape_component_t,
                      shape_component_start, shape_component_update,
                      shape_component_render, shape_component_destroy,
                      "Shape", 0, NULL);
}

// Helper constructors for common shapes (simplified to set fields only)
shape_component_t
shape_sphere_create (vec3_t position, float radius, vec4_t color)
{
  shape_component_t shape = (shape_component_t){ 0 };
  shape.transform.position = position;
  shape.type = SHAPE_SPHERE;
  shape.operation = SHAPE_OP_UNION;
  shape.dimensions.x = radius;
  shape.color = color;
  shape.roughness = 0.5f;
  shape.metallic = 0.0f;
  shape.smoothing = 0.1f;
  shape.visible = true;
  shape.dirty = true;
  return shape;
}

shape_component_t
shape_box_create (vec3_t position, vec3_t half_extents, vec4_t color)
{
  shape_component_t shape = (shape_component_t){ 0 };
  shape.transform.position = position;
  shape.type = SHAPE_BOX;
  shape.operation = SHAPE_OP_UNION;
  shape.dimensions = half_extents;
  shape.color = color;
  shape.roughness = 0.5f;
  shape.metallic = 0.0f;
  shape.smoothing = 0.1f;
  shape.visible = true;
  shape.dirty = true;
  return shape;
}

shape_component_t
shape_torus_create (vec3_t position, float major_radius, float minor_radius,
                    vec4_t color)
{
  shape_component_t shape = (shape_component_t){ 0 };
  shape.transform.position = position;
  shape.type = SHAPE_TORUS;
  shape.operation = SHAPE_OP_UNION;
  shape.dimensions.x = major_radius;
  shape.dimensions.y = minor_radius;
  shape.color = color;
  shape.roughness = 0.5f;
  shape.metallic = 0.0f;
  shape.smoothing = 0.1f;
  shape.visible = true;
  shape.dirty = true;
  return shape;
}
