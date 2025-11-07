#ifndef SHAPE_CITADEL_GLSL
#define SHAPE_CITADEL_GLSL
#include "shape_common.glsl"

vec3 orbitColor;
float
citadel_de (vec3 p)
{
  float scale = 1.4731;
  float angle1 = 0.0;
  float angle2 = 0.0;
  vec3 shift = vec3 (-10.27, 3.28, -1.90);
  vec3 color = vec3 (1.17, 0.07, 1.27);
  vec2 a1 = vec2 (sin (angle1), cos (angle1));
  vec2 a2 = vec2 (sin (angle2), cos (angle2));
  mat2 rmZ = mat2 (a1.y, a1.x, -a1.x, a1.y);
  mat2 rmX = mat2 (a2.y, a2.x, -a2.x, a2.y);
  float s = 1.0;
  orbitColor = vec3 (0.0);
  for (int i = 0; i < 15; ++i)
    {
      p = abs (p);
      p.xy *= rmZ;
      p.xy += min (p.x - p.y, 0.0) * vec2 (-1.0, 1.0);
      p.xz += min (p.x - p.z, 0.0) * vec2 (-1.0, 1.0);
      p.yz += min (p.y - p.z, 0.0) * vec2 (-1.0, 1.0);
      p.yz *= rmX;
      p *= scale;
      s *= scale;
      p += shift;
      orbitColor = max (orbitColor, p * color);
    }
  vec3 d = abs (p) - vec3 (6.0);
  return (min (max (d.x, max (d.y, d.z)), 0.0) + length (max (d, 0.0))) / s;
}

float
shape_citadel_eval (vec3 p, float seed)
{
  return 88 * citadel_de (p / 88);
}

#endif
