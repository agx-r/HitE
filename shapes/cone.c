/*
 * Cone SDF Implementation
 * 
 * Parameters:
 *   p      - Point in local space
 *   radius - Base radius
 *   height - Cone height (along Y axis, tip at top)
 * 
 * Returns: Signed distance to cone surface
 */

#include "shape_interface.h"

float
sdf_cone (vec3_t p, float radius, float height)
{
  vec2_t q = { .x = sqrtf (p.x * p.x + p.z * p.z), .y = p.y };
  vec2_t k = { .x = radius / height, .y = -1.0f };

  float d = q.x * k.x + q.y * k.y;
  if (d < 0.0f)
    {
      return sqrtf (q.x * q.x + q.y * q.y);
    }

  vec2_t c = { .x = q.x - radius, .y = q.y - height };
  if (c.y > 0.0f)
    {
      return sqrtf (c.x * c.x + c.y * c.y);
    }

  return d / sqrtf (k.x * k.x + k.y * k.y);
}
