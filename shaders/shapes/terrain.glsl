#ifndef SHAPE_TERRAIN_GLSL
#define SHAPE_TERRAIN_GLSL
#include "shape_common.glsl"

// Hash function for random values
float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

vec2 hash2_vec2(vec2 p) {
    float h = hash2(p);
    float angle = h * 6.28318; // 2*PI
    return vec2(cos(angle), sin(angle));
}

// Cheap Perlin noise (gradient noise)
float perlin_noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    // Smooth interpolation
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    // Get gradients at corners
    vec2 g00 = hash2_vec2(i + vec2(0.0, 0.0));
    vec2 g10 = hash2_vec2(i + vec2(1.0, 0.0));
    vec2 g01 = hash2_vec2(i + vec2(0.0, 1.0));
    vec2 g11 = hash2_vec2(i + vec2(1.0, 1.0));
    
    // Distance vectors from corners
    vec2 d00 = f - vec2(0.0, 0.0);
    vec2 d10 = f - vec2(1.0, 0.0);
    vec2 d01 = f - vec2(0.0, 1.0);
    vec2 d11 = f - vec2(1.0, 1.0);
    
    // Dot products
    float n00 = dot(g00, d00);
    float n10 = dot(g10, d10);
    float n01 = dot(g01, d01);
    float n11 = dot(g11, d11);
    
    // Bilinear interpolation
    float n0 = mix(n00, n10, u.x);
    float n1 = mix(n01, n11, u.x);
    return mix(n0, n1, u.y);
}

// Fractal noise (octaves)
float fractal_noise(vec2 p, float seed) {
    // Offset by seed
    p += vec2(seed * 100.0, seed * 200.0);
    
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 0.1;
    float max_value = 0.0;
    
    // 3 octaves for smooth hills
    for(int i = 0; i < 3; i++) {
        value += perlin_noise(p * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return value / max_value;
}

// Get terrain height at x,z position
float get_terrain_height(vec2 xz, float seed) {
    return fractal_noise(xz, seed) * 5.0; // Scale height to 5 units
}

float shape_terrain_eval(vec3 p, float seed) {
    // Get terrain height at this x,z position
    float terrain_height = get_terrain_height(p.xz, seed);
    
    // Distance from point to terrain surface
    // Positive if above terrain, negative if below
    return p.y - terrain_height;
}

#endif
