#ifndef SHAPE_CITADEL_GLSL
#define SHAPE_CITADEL_GLSL
#include "shape_common.glsl"

float
citadel_de (vec3 p, float time, out vec3 orbit_color)
{
  float scale = 1.4741;
  float angle1 = 0.0;
  float angle2 = 0.0;

  float a = -1.9;
  float b = 11;
  float c = 1.8;
  float d = 7.5;

  float shift_z = -2 - (sin (time * 0.04));
  vec3 shift = vec3 (-10.25, 3.37, shift_z);

  vec3 color = vec3 (0.08, 0.03, 0.03);
  vec2 a1 = vec2 (sin (angle1), cos (angle1));
  vec2 a2 = vec2 (sin (angle2), cos (angle2));
  mat2 rmZ = mat2 (a1.y, a1.x, -a1.x, a1.y);
  mat2 rmX = mat2 (a2.y, a2.x, -a2.x, a2.y);
  float s = 1.0;
  vec3 color_accum = vec3 (0.0);

  for (int i = 0; i < 11; ++i)
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

      color_accum
          = max (color_accum, abs (p) * color * (0.5 + 0.05 * float (i)));
    }

  orbit_color = color_accum;
  vec3 dvec = abs (p) - vec3 (6.0);
  return (min (max (dvec.x, max (dvec.y, dvec.z)), 0.0)
          + length (max (dvec, 0.0)))
         / s;
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
