#ifndef SHAPE_BOX_GLSL
#define SHAPE_BOX_GLSL
#include "shape_common.glsl"

float shape_box_eval(vec3 p, vec3 dimensions) {
  return sdf_box(p, dimensions);
}

#endif
