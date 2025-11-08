#ifndef SHAPE_CITADEL_GLSL
#define SHAPE_CITADEL_GLSL
#include "shape_common.glsl"

float citadel_de(vec3 p, out vec3 orbit_color, float time)
{
  float scale = 1.4741;
  float angle1 = 0.0;
  float angle2 = 0.0;
  
  float shift_z = -2.5 + 0.5 * sin(time * 0.5);
  vec3 shift = vec3(-10.29, 3.38, shift_z);
  
  vec3 color = vec3(0.05, 0.03, 0.09);
  vec2 a1 = vec2(sin(angle1), cos(angle1));
  vec2 a2 = vec2(sin(angle2), cos(angle2));
  mat2 rmZ = mat2(a1.y, a1.x, -a1.x, a1.y);
  mat2 rmX = mat2(a2.y, a2.x, -a2.x, a2.y);
  float s = 1.0;
  vec3 color_accum = vec3(0.0);
  
  for (int i = 0; i < 11; ++i)
  {
    p = abs(p);
    p.xy *= rmZ;
    p.xy += min(p.x - p.y, 0.0) * vec2(-1.0, 1.0);
    p.xz += min(p.x - p.z, 0.0) * vec2(-1.0, 1.0);
    p.yz += min(p.y - p.z, 0.0) * vec2(-1.0, 1.0);
    p.yz *= rmX;
    p *= scale;
    s *= scale;
    
    p += shift;
    
    color_accum = max(color_accum, abs(p) * color * (1.0 + 0.1 * float(i)));
  }
  
  orbit_color = color_accum;
  vec3 d = abs(p) - vec3(6.0);
  return (min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0))) / s;
}

float shape_citadel_eval(vec3 p, float size, float time, out vec3 orbit_color)
{
  vec3 local_color;
  float distance = citadel_de(p / size, local_color, time);
  orbit_color = local_color;
  return size * distance;
}

#endif
