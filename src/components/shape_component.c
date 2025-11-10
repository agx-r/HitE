#include "shape_component.h"
#include "component_registry.h"
#include <float.h>
#include <math.h>
#include <string.h>

static vec3_t
vec3_make (float x, float y, float z)
{
  return (vec3_t){ x, y, z, 0.0f };
}

static vec3_t
vec3_add (vec3_t a, vec3_t b)
{
  return vec3_make (a.x + b.x, a.y + b.y, a.z + b.z);
}

static vec3_t
vec3_sub (vec3_t a, vec3_t b)
{
  return vec3_make (a.x - b.x, a.y - b.y, a.z - b.z);
}

static vec3_t
vec3_mul_scalar (vec3_t v, float s)
{
  return vec3_make (v.x * s, v.y * s, v.z * s);
}

static vec3_t
vec3_mul_components (vec3_t a, vec3_t b)
{
  return vec3_make (a.x * b.x, a.y * b.y, a.z * b.z);
}

static vec3_t
vec3_abs (vec3_t v)
{
  return vec3_make (fabsf (v.x), fabsf (v.y), fabsf (v.z));
}

static vec3_t
vec3_max (vec3_t a, vec3_t b)
{
  return vec3_make (fmaxf (a.x, b.x), fmaxf (a.y, b.y), fmaxf (a.z, b.z));
}

static float
vec3_dot (vec3_t a, vec3_t b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float
vec3_length (vec3_t v)
{
  return sqrtf (vec3_dot (v, v));
}

static vec3_t
vec3_normalize (vec3_t v)
{
  float len = vec3_length (v);
  if (len < 1e-6f)
    return vec3_make (0.0f, 1.0f, 0.0f);
  return vec3_mul_scalar (v, 1.0f / len);
}

static vec4_t
quat_normalize (vec4_t q)
{
  float len = sqrtf (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  if (len < 1e-6f)
    return (vec4_t){ 0.0f, 0.0f, 0.0f, 1.0f };
  return (vec4_t){ q.x / len, q.y / len, q.z / len, q.w / len };
}

static vec4_t
quat_conjugate (vec4_t q)
{
  return (vec4_t){ -q.x, -q.y, -q.z, q.w };
}

static vec4_t
quat_multiply (vec4_t a, vec4_t b)
{
  return (vec4_t){ a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                   a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                   a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                   a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z };
}

static vec3_t
quat_rotate (vec4_t q, vec3_t v)
{
  vec4_t normalized = quat_normalize (q);
  vec4_t v_quat = { v.x, v.y, v.z, 0.0f };
  vec4_t rotated = quat_multiply (quat_multiply (normalized, v_quat),
                                  quat_conjugate (normalized));
  return vec3_make (rotated.x, rotated.y, rotated.z);
}

vec3_t
shape_transform_point (vec3_t point, const transform_t *transform)
{
  if (!transform)
    return point;

  vec3_t scaled = vec3_mul_components (point, transform->scale);
  vec3_t rotated = quat_rotate (transform->rotation, scaled);
  return vec3_add (rotated, transform->position);
}

vec3_t
shape_transform_point_inverse (vec3_t point, const transform_t *transform)
{
  if (!transform)
    return point;

  vec3_t translated = vec3_sub (point, transform->position);
  vec4_t conj = quat_conjugate (transform->rotation);
  vec3_t rotated = quat_rotate (conj, translated);

  vec3_t scale = transform->scale;
  float sx = fabsf (scale.x) < 1e-6f ? 1.0f : scale.x;
  float sy = fabsf (scale.y) < 1e-6f ? 1.0f : scale.y;
  float sz = fabsf (scale.z) < 1e-6f ? 1.0f : scale.z;

  return vec3_make (rotated.x / sx, rotated.y / sy, rotated.z / sz);
}

static float
sdf_sphere (vec3_t p, float r)
{
  return vec3_length (p) - r;
}

static float
sdf_box (vec3_t p, vec3_t b)
{
  vec3_t q = vec3_sub (vec3_abs (p), b);
  vec3_t max_q = vec3_max (q, vec3_make (0.0f, 0.0f, 0.0f));
  float outside = vec3_length (max_q);
  float inside = fminf (fmaxf (q.x, fmaxf (q.y, q.z)), 0.0f);
  return outside + inside;
}

static float
sdf_torus (vec3_t p, float major_radius, float minor_radius)
{
  float qx = sqrtf (p.x * p.x + p.z * p.z) - major_radius;
  float qy = p.y;
  return sqrtf (qx * qx + qy * qy) - minor_radius;
}

static float
sdf_plane (vec3_t p, vec3_t n, float h)
{
  vec3_t normal = vec3_normalize (n);
  return vec3_dot (p, normal) + h;
}

static float
fractf (float x)
{
  return x - floorf (x);
}

static float
smoothstepf (float edge0, float edge1, float x)
{
  if (edge0 == edge1)
    return x < edge0 ? 0.0f : 1.0f;

  float t = (x - edge0) / (edge1 - edge0);
  t = fmaxf (0.0f, fminf (1.0f, t));
  return t * t * (3.0f - 2.0f * t);
}

static float
snoise (vec2_t p)
{
  vec2_t f = { fractf (p.x), fractf (p.y), { 0.0f, 0.0f } };
  vec2_t base = { floorf (p.x), floorf (p.y), { 0.0f, 0.0f } };

  float v = base.x + base.y * 1000.0f;
  float r0 = fractf (100000.0f * sinf ((v) * 0.001f));
  float r1 = fractf (100000.0f * sinf ((v + 1.0f) * 0.001f));
  float r2 = fractf (100000.0f * sinf ((v + 1000.0f) * 0.001f));
  float r3 = fractf (100000.0f * sinf ((v + 1001.0f) * 0.001f));

  f.x = f.x * f.x * (3.0f - 2.0f * f.x);
  f.y = f.y * f.y * (3.0f - 2.0f * f.y);

  float i1 = r0 + (r1 - r0) * f.x;
  float i2 = r2 + (r3 - r2) * f.x;
  return 2.0f * (i1 + (i2 - i1) * f.y) - 1.0f;
}

static float
terrain (vec2_t p, int octaves)
{
  float h = 0.0f;
  float w = 0.5f;
  float m = 0.4f;
  for (int i = 0; i < octaves && i < 12; ++i)
    {
      vec2_t scaled = { p.x * m, p.y * m, { 0.0f, 0.0f } };
      h += w * snoise (scaled);
      w *= 0.5f;
      m *= 2.0f;
    }
  return h;
}

static float
terrain_sand (vec2_t p, int octaves)
{
  float h = 0.0f;
  float f = 1.0f;
  for (int i = 0; i < octaves && i < 12; ++i)
    {
      vec2_t scaled = { p.x * f, p.y * f, { 0.0f, 0.0f } };
      h += fabsf (snoise (scaled) / f);
      f *= 2.0f;
    }
  return h;
}

static float
terrain_de (vec3_t p)
{
  float d_min = 28.0f;
  int octaves = 9;

  vec2_t p_xz = { p.x, p.z, { 0.0f, 0.0f } };
  float h = terrain (p_xz, octaves);
  h += smoothstepf (0.0f, 1.1f, h);
  h += smoothstepf (-0.1f, 1.0f, p.y) * 0.6f;
  float d = p.y - h;

  if (d < d_min)
    d_min = d;

  if (h < 0.5f)
    {
      vec2_t sand_p = { p.x * 0.2f, p.z * 0.2f, { 0.0f, 0.0f } };
      float s = 0.3f * terrain_sand (sand_p, octaves);
      d = p.y - 0.35f + s;
      if (d < d_min)
        d_min = d;
    }

  return d_min;
}

static float
shape_terrain_eval (vec3_t p)
{
  return 100.0f * terrain_de (vec3_mul_scalar (p, 0.01f));
}

static float
citadel_de (vec3_t p, float time, vec3_t *out_color)
{
  (void)time;

  const float scale = 1.4741f;
  const vec3_t shift = { -10.25f, 3.37f, -2.0f, 0.0f };
  const vec3_t color = { 0.08f, 0.03f, 0.03f, 0.0f };

  float angle1 = 0.0f;
  float angle2 = 0.0f;
  float sin1 = sinf (angle1);
  float cos1 = cosf (angle1);
  float sin2 = sinf (angle2);
  float cos2 = cosf (angle2);

  float s = 1.0f;
  vec3_t color_accum = vec3_make (0.0f, 0.0f, 0.0f);

  for (int i = 0; i < 11; ++i)
    {
      p = vec3_abs (p);

      float min_xy = fminf (p.x - p.y, 0.0f);
      p.x += -min_xy;
      p.y += min_xy;

      float min_xz = fminf (p.x - p.z, 0.0f);
      p.x += -min_xz;
      p.z += min_xz;

      float min_yz = fminf (p.y - p.z, 0.0f);
      p.y += -min_yz;
      p.z += min_yz;

      float new_x = p.x * cos1 + p.y * sin1;
      float new_y = -p.x * sin1 + p.y * cos1;
      p.x = new_x;
      p.y = new_y;

      float new_y2 = p.y * cos2 + p.z * sin2;
      float new_z2 = -p.y * sin2 + p.z * cos2;
      p.y = new_y2;
      p.z = new_z2;

      p = vec3_mul_scalar (p, scale);
      s *= scale;

      p = vec3_add (p, shift);

      vec3_t abs_p = vec3_abs (p);
      vec3_t scaled_color = vec3_mul_components (
          abs_p, vec3_mul_scalar (color, 0.5f + 0.05f * i));
      color_accum = vec3_max (color_accum, scaled_color);
    }

  if (out_color)
    *out_color = color_accum;

  vec3_t dvec = vec3_sub (vec3_abs (p), vec3_make (6.0f, 6.0f, 6.0f));
  float max_component = fmaxf (dvec.x, fmaxf (dvec.y, dvec.z));
  float inside = fminf (max_component, 0.0f);
  vec3_t max_zero = vec3_max (dvec, vec3_make (0.0f, 0.0f, 0.0f));
  float outside = vec3_length (max_zero);

  return (inside + outside) / s;
}

static float
shape_citadel_eval (vec3_t p, float size, float time)
{
  vec3_t dummy = vec3_make (0.0f, 0.0f, 0.0f);
  float distance = citadel_de (vec3_mul_scalar (p, 1.0f / size), time, &dummy);
  return size * distance;
}

float
shape_evaluate_sdf (const shape_component_t *shape, vec3_t world_point,
                    float time_seconds)
{
  if (!shape)
    return FLT_MAX;

  vec3_t local_point
      = shape_transform_point_inverse (world_point, &shape->transform);

  switch (shape->type)
    {
    case SHAPE_SPHERE:
      {
        float radius = shape->dimensions.x != 0.0f ? shape->dimensions.x
                                                   : fmaxf (shape->size, 0.0f);
        return sdf_sphere (local_point, radius);
      }
    case SHAPE_BOX:
      return sdf_box (local_point, shape->dimensions);
    case SHAPE_TORUS:
      return sdf_torus (local_point, shape->dimensions.x,
                        shape->dimensions.y != 0.0f ? shape->dimensions.y
                                                    : shape->size);
    case SHAPE_PLANE:
      {
        vec3_t normal = shape->dimensions;
        if (vec3_length (normal) < 1e-4f)
          normal = vec3_make (0.0f, 1.0f, 0.0f);
        float distance = shape->size;
        return sdf_plane (local_point, normal, distance);
      }
    case SHAPE_TERRAIN:
      return shape_terrain_eval (local_point);
    case SHAPE_CITADEL:
      return shape_citadel_eval (local_point, fmaxf (shape->size, 0.001f),
                                 time_seconds);
    case SHAPE_CUSTOM:
      if (shape->custom_sdf)
        return shape->custom_sdf (local_point, shape->custom_params);
      break;
    default:
      break;
    }

  return FLT_MAX;
}

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
  REGISTER_COMPONENT (world, "shape", shape_component_t, shape_component_start,
                      shape_component_update, shape_component_render,
                      shape_component_destroy, "Shape", 0, NULL);
}
