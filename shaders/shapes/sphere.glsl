#ifndef SHAPE_SPHERE_GLSL
#define SHAPE_SPHERE_GLSL
#include "shape_common.glsl"

float
shape_sphere_eval (vec3 p, float radius, float time)
{
  return sdf_sphere (p, radius);
}

#endif
