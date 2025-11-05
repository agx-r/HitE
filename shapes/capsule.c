/*
 * Capsule SDF Implementation
 * 
 * Parameters:
 *   p      - Point in local space
 *   radius - Capsule radius
 *   height - Distance between hemisphere centers (along Y axis)
 * 
 * Returns: Signed distance to capsule surface
 */

#include "shape_interface.h"

float
sdf_capsule (vec3_t p, float radius, float height)
{
  p.y -= clampf (p.y, -height * 0.5f, height * 0.5f);
  return vec3_length (p) - radius;
}
