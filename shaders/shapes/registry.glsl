#ifndef SHAPES_REGISTRY_GLSL
#define SHAPES_REGISTRY_GLSL

#include "shape_common.glsl"
#include "sphere.glsl"
#include "box.glsl"
#include "torus.glsl"
#include "plane.glsl"
#include "terrain.glsl"

// To add a new shape
// Create shapes/your_shape.glsl with shape_your_shape_eval(vec3 p, param1, param2, ...)
// Include it above
// Add one line here: if (t == TYPE) return shape_your_shape_eval(local_p, obj.dimensions.x, obj.params.y, ...);
float eval_shape(vec3 local_p, vec4 position, vec4 dimensions, vec4 params) {
  uint t = uint(dimensions.w);
  
  if (t == 0u) return shape_sphere_eval(local_p, position.w);
  if (t == 1u) return shape_box_eval(local_p, dimensions.xyz);
  if (t == 2u) return shape_torus_eval(local_p, dimensions.xy);
  if (t == 3u) return shape_plane_eval(local_p, dimensions.xyz, dimensions.w);
  if (t == 7u) return shape_terrain_eval(local_p, params.x); // params.x = seed
  
  return 1e9;
}

#endif
