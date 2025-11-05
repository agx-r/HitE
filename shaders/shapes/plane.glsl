#ifndef SHAPE_PLANE_GLSL
#define SHAPE_PLANE_GLSL
#include "shape_common.glsl"

float shape_plane_eval(vec3 p, vec3 normal, float distance) {
  return sdf_plane(p, normal, distance);
}

#endif
