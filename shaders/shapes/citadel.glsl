#ifndef SHAPE_CITADEL_GLSL
#define SHAPE_CITADEL_GLSL
#include "shape_common.glsl"

float
citadel_de (vec3 p, float time, out vec3 orbit_color)
{
  const float scale = 1.4721;
  const vec3 shift = vec3 (-10.23, 3.35, -2);
  const vec3 base_color = vec3 (0.08, 0.03, 0.03);

  float s = 1.0;
  vec3 color_accum = vec3 (0.0);

  for (int i = 0; i < 13; ++i)
    {
      p = abs (p);
      p.xy += min (p.x - p.y, 0.0) * vec2 (-1.0, 1.0);
      p.xz += min (p.x - p.z, 0.0) * vec2 (-1.0, 1.0);
      p.yz += min (p.y - p.z, 0.0) * vec2 (-1.0, 1.0);

      p = p * scale + shift;
      s *= scale;

      color_accum
          = max (color_accum, abs (p) * base_color * (0.5 + 0.05 * float (i)));
    }

  orbit_color = color_accum;
  vec3 d = abs (p) - vec3 (6.0);
  return (min (max (d.x, max (d.y, d.z)), 0.0) + length (max (d, 0.0))) / s;
}

float
shape_citadel_eval (vec3 p, float size, float time, out vec3 orbit_color)
{
  vec3 local_color;
  float distance = citadel_de (p / size, time, local_color);
  orbit_color = local_color;
  return size * distance;
}

#endif
