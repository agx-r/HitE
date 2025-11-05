/*
 * Plane SDF Implementation
 * 
 * Parameters:
 *   p        - Point in local space
 *   normal   - Plane normal vector (should be normalized)
 *   distance - Distance from origin along normal
 * 
 * Returns: Signed distance to plane surface
 */

#include "shape_interface.h"

float
sdf_plane (vec3_t p, vec3_t normal, float distance)
{
  return vec3_dot (p, normal) + distance;
}
