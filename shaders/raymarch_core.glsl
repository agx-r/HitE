#ifndef RAYMARCH_CORE_GLSL
#define RAYMARCH_CORE_GLSL

#include "shapes/registry.glsl"

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

// Raymarch result structure
struct RaymarchResult {
  vec4 color;      // RGB = surface color, A = depth
  vec4 normal_pos; // RGB = normal (normalized), A = hit flag (1.0 = hit, 0.0 = miss)
};

// Main raymarching loop
RaymarchResult raymarch(vec3 ro, vec3 rd, vec3 camera_pos) {
  float depth = 0.0;
  vec4 color = vec4(0.0);
  RaymarchResult result;

  for (int i = 0; i < MAX_STEPS; i++) {
    vec3 p = ro + rd * depth;
    float dist = scene_sdf(p, color);

    if (dist < EPSILON) {
      vec3 normal = calculate_normal(p);
      vec3 hit_pos = p;
      // Return color without lighting - lighting will be applied in post-pass
      // Store depth in color.a
      result.color = vec4(color.rgb, depth);
      // Store normal in RGB, hit flag in A
      result.normal_pos = vec4(normal * 0.5 + 0.5, 1.0); // Normalize to 0..1 range
      return result;
    }

    depth += dist;
    if (depth >= MAX_DIST) break;
  }

  // Background gradient
  float t = rd.y * 0.5 + 0.5;
  vec3 bg = mix(vec3(0.06, 0.06, 0.06), vec3(0.1, 0.1, 0.1), t);
  result.color = vec4(bg, MAX_DIST);
  result.normal_pos = vec4(0.5, 0.5, 1.0, 0.0); // No hit
  return result;
}

#endif
