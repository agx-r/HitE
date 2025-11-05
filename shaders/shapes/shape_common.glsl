#ifndef SHAPE_COMMON_GLSL
#define SHAPE_COMMON_GLSL

// SDFObject is defined in raymarch.comp, we just declare functions here

// Basic SDF primitives (used by other shapes)
float sdf_sphere(vec3 p, float r) {
  return length(p) - r;
}

float sdf_box(vec3 p, vec3 b) {
  vec3 q = abs(p) - b;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdf_torus(vec3 p, vec2 t) {
  vec2 q = vec2(length(p.xz) - t.x, p.y);
  return length(q) - t.y;
}

float sdf_plane(vec3 p, vec3 n, float h) {
  return dot(p, n) + h;
}

// Smooth blending
float smooth_min(float a, float b, float k) {
  float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
  return mix(b, a, h) - k * h * (1.0 - h);
}

// Math utilities
float hash(float n) {
  return fract(sin(n) * 43758.5453);
}

float fractf(float x) {
  return fract(x);
}

#endif
