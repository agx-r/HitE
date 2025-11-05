#include "shape_component.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Math helpers
static inline float
vec3_length (vec3_t v)
{
  return sqrtf (v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline vec3_t
vec3_sub (vec3_t a, vec3_t b)
{
  return (vec3_t){ a.x - b.x, a.y - b.y, a.z - b.z, 0 };
}

static inline vec3_t
vec3_abs (vec3_t v)
{
  return (vec3_t){ fabsf (v.x), fabsf (v.y), fabsf (v.z), 0 };
}

static inline vec3_t
vec3_max_scalar (vec3_t v, float s)
{
  return (vec3_t){ fmaxf (v.x, s), fmaxf (v.y, s), fmaxf (v.z, s), 0 };
}

static inline float
vec3_max_component (vec3_t v)
{
  return fmaxf (v.x, fmaxf (v.y, v.z));
}

static inline float
vec3_dot (vec3_t a, vec3_t b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float
clampf (float x, float min, float max)
{
  return fmaxf (min, fminf (max, x));
}

// SDF primitives implementation
float
sdf_sphere (vec3_t p, float radius)
{
  return vec3_length (p) - radius;
}

float
sdf_box (vec3_t p, vec3_t half_extents)
{
  vec3_t q = vec3_sub (vec3_abs (p), half_extents);
  return vec3_length (vec3_max_scalar (q, 0.0f))
         + fminf (vec3_max_component (q), 0.0f);
}

float
sdf_torus (vec3_t p, float major_radius, float minor_radius)
{
  float xz_length = sqrtf (p.x * p.x + p.z * p.z);
  vec2_t q = { xz_length - major_radius, p.y, 0, 0 };
  return sqrtf (q.x * q.x + q.y * q.y) - minor_radius;
}

float
sdf_plane (vec3_t p, vec3_t normal, float distance)
{
  return vec3_dot (p, normal) + distance;
}

float
sdf_cylinder (vec3_t p, float radius, float height)
{
  float xz_length = sqrtf (p.x * p.x + p.z * p.z);
  vec2_t d = { xz_length - radius, fabsf (p.y) - height * 0.5f, 0, 0 };
  return fminf (fmaxf (d.x, d.y), 0.0f)
         + vec3_length (
             (vec3_t){ fmaxf (d.x, 0.0f), fmaxf (d.y, 0.0f), 0, 0 });
}

float
sdf_capsule (vec3_t p, float radius, float height)
{
  p.y -= clampf (p.y, -height * 0.5f, height * 0.5f);
  return vec3_length (p) - radius;
}

float
sdf_cone (vec3_t p, float radius, float height)
{
  vec2_t q = { sqrtf (p.x * p.x + p.z * p.z), p.y, 0, 0 };
  vec2_t k = { radius / height, -1.0f, 0, 0 };

  float d = q.x * k.x + q.y * k.y;
  if (d < 0.0f)
    {
      return sqrtf (q.x * q.x + q.y * q.y);
    }

  vec2_t c = { q.x - radius, q.y - height, 0, 0 };
  if (c.y > 0.0f)
    {
      return sqrtf (c.x * c.x + c.y * c.y);
    }

  return d / sqrtf (k.x * k.x + k.y * k.y);
}

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
  return (vec3_t){ point.x + transform->position.x,
                   point.y + transform->position.y,
                   point.z + transform->position.z, 0 };
}

vec3_t
transform_point_inverse (vec3_t point, const transform_t *transform)
{
  // Inverse transformation: subtract position
  // Full implementation would apply inverse rotation and scale
  return (vec3_t){ point.x - transform->position.x,
                   point.y - transform->position.y,
                   point.z - transform->position.z, 0 };
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
      distance = sdf_plane (local_point, (vec3_t){ 0, 1, 0, 0 },
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
      shape->color = (vec4_t){ 0.8f, 0.8f, 0.8f, 1.0f };
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
  shape.transform.scale = (vec3_t){ 1, 1, 1, 0 };
  shape.transform.rotation = (vec4_t){ 0, 0, 0, 1 }; // Identity quaternion
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
  shape.transform.scale = (vec3_t){ 1, 1, 1, 0 };
  shape.transform.rotation = (vec4_t){ 0, 0, 0, 1 };
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
  shape.transform.scale = (vec3_t){ 1, 1, 1, 0 };
  shape.transform.rotation = (vec4_t){ 0, 0, 0, 1 };
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
