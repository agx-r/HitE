/*
 * Sphere SDF Implementation
 * 
 * Parameters:
 *   p      - Point in local space
 *   radius - Sphere radius
 * 
 * Returns: Signed distance to sphere surface
 */

#include "shape_interface.h"

float
sdf_sphere (vec3_t p, float radius)
{
  return vec3_length (p) - radius;
}
