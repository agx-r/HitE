#ifndef SHAPE_TERRAIN_GLSL
#define SHAPE_TERRAIN_GLSL
#include "shape_common.glsl"

// Sphere SDF for pillars
float sdf_sphere_local(vec3 p, float r) {
  return length(p) - r;
}

// Box SDF for arch beam
float sdf_box_local(vec3 p, vec3 b) {
  float dx = abs(p.x) - b.x;
  float dy = abs(p.y) - b.y;
  float dz = abs(p.z) - b.z;
  float dxp = max(dx, 0.0);
  float dyp = max(dy, 0.0);
  float dzp = max(dz, 0.0);
  return min(max(max(dx, dy), dz), 0.0) + length(vec3(dxp, dyp, dzp));
}

float shape_terrain_eval(vec3 p, float height_scale, float noise_scale, float arch_spacing, float arch_height, float arch_width) {

  // Flat base plane (no noise)
  float base_plane = p.y;
  float terrain_dist = base_plane;
  
  // Create arches at EVERY grid position
  float arch_cell_x = floor((p.x + arch_spacing * 0.5) / arch_spacing);
  float arch_cell_z = floor((p.z + arch_spacing * 0.5) / arch_spacing);
  
  float arch_x = arch_cell_x * arch_spacing;
  float arch_z = arch_cell_z * arch_spacing;
  
  vec3 arch_p = vec3(p.x - arch_x, p.y, p.z - arch_z);
  
  // Arch structure
  float pillar_radius = arch_width * 0.15;
  float pillar_height = arch_height;
  
  // Left pillar
  vec3 left_p = vec3(arch_p.x + arch_width * 0.5, arch_p.y - pillar_height * 0.5, arch_p.z);
  float left_pillar_dist = sdf_sphere_local(left_p, pillar_radius);
  
  // Right pillar
  vec3 right_p = vec3(arch_p.x - arch_width * 0.5, arch_p.y - pillar_height * 0.5, arch_p.z);
  float right_pillar_dist = sdf_sphere_local(right_p, pillar_radius);
  
  // Top beam
  vec3 top_p = vec3(arch_p.x, arch_p.y - arch_height, arch_p.z);
  vec3 beam_size = vec3(arch_width * 1.1, pillar_radius * 1.5, pillar_radius * 1.5);
  float top_beam_dist = sdf_box_local(top_p, beam_size);
  
  float arch_structure = min(min(left_pillar_dist, right_pillar_dist), top_beam_dist);
  
  // Explicit hole/passage
  vec3 hole_p = vec3(arch_p.x, arch_p.y - arch_height * 0.3, arch_p.z);
  float hole_radius = arch_width * 0.6;
  float hole_height = arch_height * 1.2;
  
  float hole_xz_dist = length(hole_p.xz) - hole_radius;
  float hole_y_dist = abs(hole_p.y) - hole_height * 0.5;
  float hole_dist = max(hole_xz_dist, hole_y_dist);
  
  float arch_with_hole = max(arch_structure, -hole_dist);
  terrain_dist = min(terrain_dist, arch_with_hole);
  terrain_dist = max(terrain_dist, -hole_dist * 0.8);
  
  return terrain_dist;
}

#endif
