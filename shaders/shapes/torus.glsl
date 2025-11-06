#ifndef SHAPE_TORUS_GLSL
#define SHAPE_TORUS_GLSL
#include "shape_common.glsl"

float
shape_torus_eval (vec3 p, vec2 dimensions)
{
  return sdf_torus (p, dimensions);
}

#endif
