/*
 * Torus SDF Implementation
 * 
 * Parameters:
 *   p            - Point in local space
 *   major_radius - Distance from center to tube center
 *   minor_radius - Tube radius
 * 
 * Returns: Signed distance to torus surface
 */

#include "shape_interface.h"

float
sdf_torus (vec3_t p, float major_radius, float minor_radius)
{
  float xz_length = sqrtf (p.x * p.x + p.z * p.z);
  vec2_t q = { .x = xz_length - major_radius, .y = p.y };
  return sqrtf (q.x * q.x + q.y * q.y) - minor_radius;
}
