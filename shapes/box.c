/*
 * Box SDF Implementation
 * 
 * Parameters:
 *   p            - Point in local space
 *   half_extents - Half dimensions (width/2, height/2, depth/2)
 * 
 * Returns: Signed distance to box surface
 */

#include "shape_interface.h"

float
sdf_box (vec3_t p, vec3_t half_extents)
{
  vec3_t q = vec3_sub (vec3_abs (p), half_extents);
  return vec3_length (vec3_max_scalar (q, 0.0f))
         + fminf (vec3_max_component (q), 0.0f);
}
