#include "shape_component.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include all shape implementations from shapes directory
// They bring their own math helpers from shape_interface.h
#include "../../shapes/box.c"
#include "../../shapes/capsule.c"
#include "../../shapes/cone.c"
#include "../../shapes/cylinder.c"
#include "../../shapes/plane.c"
#include "../../shapes/sphere.c"
#include "../../shapes/torus.c"

// Shape operations
float
sdf_union (float d1, float d2)
{
  return fminf (d1, d2);
}

float
sdf_subtraction (float d1, float d2)
{
  return fmaxf (d1, -d2);
}

float
sdf_intersection (float d1, float d2)
{
  return fmaxf (d1, d2);
}

float
sdf_smooth_union (float d1, float d2, float k)
{
  float h = clampf (0.5f + 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
  return d1 * (1.0f - h) + d2 * h - k * h * (1.0f - h);
}

// Transform helpers (simplified - full implementation would use quaternions)
vec3_t
transform_point (vec3_t point, const transform_t *transform)
{
  // Simple translation for now
  // Full implementation would apply rotation (quaternion) and scale
  return (vec3_t){ .x = point.x + transform->position.x,
                   .y = point.y + transform->position.y,
                   .z = point.z + transform->position.z,
                   ._padding = 0 };
}

vec3_t
transform_point_inverse (vec3_t point, const transform_t *transform)
{
  // Inverse transformation: subtract position
  // Full implementation would apply inverse rotation and scale
  return (vec3_t){ .x = point.x - transform->position.x,
                   .y = point.y - transform->position.y,
                   .z = point.z - transform->position.z,
                   ._padding = 0 };
}

// Evaluate shape's SDF
float
shape_evaluate_sdf (const shape_component_t *shape, vec3_t world_point)
{
  // Transform to local space
  vec3_t local_point
      = transform_point_inverse (world_point, &shape->transform);

  float distance;

  // Evaluate based on shape type
  switch (shape->type)
    {
    case SHAPE_SPHERE:
      distance = sdf_sphere (local_point, shape->dimensions.x);
      break;

    case SHAPE_BOX:
      distance = sdf_box (local_point, shape->dimensions);
      break;

    case SHAPE_TORUS:
      distance
          = sdf_torus (local_point, shape->dimensions.x, shape->dimensions.y);
      break;

    case SHAPE_PLANE:
      distance = sdf_plane (local_point,
                            (vec3_t){ .x = 0, .y = 1, .z = 0, ._padding = 0 },
                            shape->dimensions.x);
      break;

    case SHAPE_CYLINDER:
      distance = sdf_cylinder (local_point, shape->dimensions.x,
                               shape->dimensions.y);
      break;

    case SHAPE_CAPSULE:
      distance = sdf_capsule (local_point, shape->dimensions.x,
                              shape->dimensions.y);
      break;

    case SHAPE_CONE:
      distance
          = sdf_cone (local_point, shape->dimensions.x, shape->dimensions.y);
      break;

    case SHAPE_CUSTOM:
      if (shape->custom_sdf)
        {
          distance = shape->custom_sdf (local_point, shape->custom_params);
        }
      else
        {
          distance = 1000.0f; // Far away
        }
      break;

    default:
      distance = 1000.0f;
      break;
    }

  return distance;
}

// Component lifecycle
result_t
shape_component_start (ecs_world_t *world, entity_id_t entity,
                       void *component_data)
{
  (void)world;

  shape_component_t *shape = (shape_component_t *)component_data;

  // Initialize defaults if needed
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

  // shape_component_t *shape = (shape_component_t *)component_data;

  // Mark dirty if transform changed (simplified - in real impl would check
  // change flags) This is where you could animate shapes procedurally

  return RESULT_SUCCESS;
}

result_t
shape_component_render (ecs_world_t *world, entity_id_t entity,
                        const void *component_data)
{
  (void)world;
  (void)entity;

  const shape_component_t *shape = (const shape_component_t *)component_data;

  // This will be called to collect shapes for GPU upload
  // The actual rendering happens in the raymarcher

  if (!shape->visible)
    {
      return RESULT_SUCCESS;
    }

  // Shape data is already in component, ready for GPU transfer

  return RESULT_SUCCESS;
}

void
shape_component_destroy (void *component_data)
{
  shape_component_t *shape = (shape_component_t *)component_data;

  if (shape->custom_params)
    {
      free (shape->custom_params);
      shape->custom_params = NULL;
    }
}

// Register component
void
shape_component_register (ecs_world_t *world)
{
  component_descriptor_t descriptor = { 0 };
  descriptor.name = "shape";
  descriptor.data_size = sizeof (shape_component_t);
  descriptor.alignment = 64; // GPU alignment
  descriptor.dependencies = NULL;
  descriptor.start = shape_component_start;
  descriptor.update = shape_component_update;
  descriptor.render = shape_component_render;
  descriptor.destroy = shape_component_destroy;

  component_id_t id;
  result_t result = ecs_register_component (world, &descriptor, &id);

  if (result.code != RESULT_OK)
    {
      fprintf (stderr, "[Error] Failed to register shape component: %s\n",
               result.message);
    }
}

// Helper constructors
shape_component_t
shape_sphere_create (vec3_t position, float radius, vec4_t color)
{
  shape_component_t shape = { 0 };
  shape.transform.position = position;
  shape.transform.scale = (vec3_t){ .x = 1, .y = 1, .z = 1, ._padding = 0 };
  shape.transform.rotation
      = (vec4_t){ .x = 0, .y = 0, .z = 0, .w = 1 }; // Identity quaternion
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
  shape_component_t shape = { 0 };
  shape.transform.position = position;
  shape.transform.scale = (vec3_t){ .x = 1, .y = 1, .z = 1, ._padding = 0 };
  shape.transform.rotation = (vec4_t){ .x = 0, .y = 0, .z = 0, .w = 1 };
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
  shape_component_t shape = { 0 };
  shape.transform.position = position;
  shape.transform.scale = (vec3_t){ .x = 1, .y = 1, .z = 1, ._padding = 0 };
  shape.transform.rotation = (vec4_t){ .x = 0, .y = 0, .z = 0, .w = 1 };
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
