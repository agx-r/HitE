#ifndef RAYMARCH_CORE_GLSL
#define RAYMARCH_CORE_GLSL

#include "shapes/registry.glsl"
#include "lighting.glsl"

// Raymarching constants
const int MAX_STEPS = 128;
const float MAX_DIST = 100.0;
const float EPSILON = 0.001;

// Scene evaluation - iterates through all objects
// Note: SDFObject, sdf_objects, ubo are defined in raymarch.comp
float scene_sdf(vec3 p, out vec4 color) {
  float min_dist = MAX_DIST;
  color = vec4(0.1, 0.1, 0.1, 1.0);

  for (uint i = 0u; i < ubo.object_count; i++) {
    SDFObject obj = sdf_objects.objects[i];
    vec3 local_p = p - obj.position.xyz;
    
    float dist = eval_shape(local_p, obj.position, obj.dimensions, obj.params);
    
    if (dist < min_dist) {
      min_dist = dist;
      color = obj.color;
    }
  }

  return min_dist;
}

// Calculate normal using tetrahedron technique
vec3 calculate_normal(vec3 p) {
  vec4 dummy_color;
  const float h = EPSILON;
  const vec2 k = vec2(1, -1);
  return normalize(
    k.xyy * scene_sdf(p + k.xyy * h, dummy_color) +
    k.yyx * scene_sdf(p + k.yyx * h, dummy_color) +
    k.yxy * scene_sdf(p + k.yxy * h, dummy_color) +
    k.xxx * scene_sdf(p + k.xxx * h, dummy_color)
  );
}

// Main raymarching loop
vec4 raymarch(vec3 ro, vec3 rd, vec3 camera_pos) {
  float depth = 0.0;
  vec4 color = vec4(0.0);

  for (int i = 0; i < MAX_STEPS; i++) {
    vec3 p = ro + rd * depth;
    float dist = scene_sdf(p, color);

    if (dist < EPSILON) {
      vec3 normal = calculate_normal(p);
      vec3 lit_color = apply_lighting(p, normal, color.rgb, camera_pos);
      return vec4(lit_color, 1.0);
    }

    depth += dist;
    if (depth >= MAX_DIST) break;
  }

  // Background gradient
  float t = rd.y * 0.5 + 0.5;
  vec3 bg = mix(vec3(0.06, 0.06, 0.06), vec3(0.1, 0.1, 0.1), t);
  return vec4(bg, 1.0);
}

#endif
