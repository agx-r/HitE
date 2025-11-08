#ifndef SHAPE_TOWN_GLSL
#define SHAPE_TOWN_GLSL
#include "shape_common.glsl"

float
town_de (vec3 p)
{
  p.xz = mod (p.xz, 2.0) - 1.0;
  vec3 q = p;
  float s = 2.0, e;
  for (int j = 0; j++ < 8;)
    {
      s *= e = 2.0 / clamp (dot (p, p), 0.5, 1.0);
      p = abs (p) * e - vec3 (0.5, 8.0, 0.5);
    }
  return max (q.y, length (p.xz) / s);
}

float
shape_town_eval (vec3 p, float seed, float time)
{
  return town_de (p);
}

#endif
