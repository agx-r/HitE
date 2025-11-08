#ifndef SHAPES_REGISTRY_GLSL
#define SHAPES_REGISTRY_GLSL

#include "box.glsl"
#include "citadel.glsl"
#include "plane.glsl"
#include "shape_common.glsl"
#include "sphere.glsl"
#include "terrain.glsl"
#include "torus.glsl"
#include "town.glsl"

float
eval_shape (vec3 local_p, vec4 position, vec4 dimensions, vec4 params,
            out vec3 dynamic_color, out bool has_dynamic_color)
{
  dynamic_color = vec3 (0.0);
  has_dynamic_color = false;

  uint t = uint (dimensions.w);

  if (t == 0u)
    return shape_sphere_eval (local_p, position.w);
  if (t == 1u)
    return shape_box_eval (local_p, dimensions.xyz);
  if (t == 2u)
    return shape_torus_eval (local_p, dimensions.xy);
  if (t == 3u)
    return shape_plane_eval (local_p, dimensions.xyz, dimensions.w);
  if (t == 7u)
    return shape_terrain_eval (local_p, params.x);
  if (t == 8u)
    {
      vec3 orbit_color;
      float distance = shape_citadel_eval (local_p, params.y, orbit_color);
      has_dynamic_color = true;
      float intensity = max (params.w, 1.0);
      dynamic_color = orbit_color * intensity;
      return distance;
    }
  if (t == 9u)
    return shape_town_eval (local_p, params.x);

  return 1e9;
}

#endif
