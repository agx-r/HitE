/*
 * Cylinder SDF Implementation
 * 
 * Parameters:
 *   p      - Point in local space
 *   radius - Cylinder radius
 *   height - Cylinder height (centered on Y axis)
 * 
 * Returns: Signed distance to cylinder surface
 */

#include "shape_interface.h"

float
sdf_cylinder (vec3_t p, float radius, float height)
{
  float xz_length = sqrtf (p.x * p.x + p.z * p.z);
  vec2_t d = { .x = xz_length - radius, .y = fabsf (p.y) - height * 0.5f };
  return fminf (fmaxf (d.x, d.y), 0.0f)
         + vec3_length (
             (vec3_t){ .x = fmaxf (d.x, 0.0f), .y = fmaxf (d.y, 0.0f), .z = 0, ._padding = 0 });
}
