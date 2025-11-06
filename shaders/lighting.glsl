#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

struct LightingConfig
{
  vec3 light_pos;
  float ambient;
  float diffuse_strength;
  float specular_strength;
  float specular_power;
};

LightingConfig
get_default_lighting ()
{
  LightingConfig cfg;
  cfg.light_pos = vec3 (5.0, 10.0, 5.0);
  cfg.ambient = 0.2;
  cfg.diffuse_strength = 0.6;
  cfg.specular_strength = 0.3;
  cfg.specular_power = 32.0;
  return cfg;
}

vec3
apply_lighting (vec3 p, vec3 normal, vec3 color, vec3 camera_pos,
                LightingConfig cfg)
{
  vec3 light_dir = normalize (cfg.light_pos - p);
  float diffuse = max (dot (normal, light_dir), 0.0);

  vec3 view_dir = normalize (camera_pos - p);
  vec3 reflect_dir = reflect (-light_dir, normal);
  float specular
      = pow (max (dot (view_dir, reflect_dir), 0.0), cfg.specular_power);

  return color * (cfg.ambient + cfg.diffuse_strength * diffuse)
         + vec3 (cfg.specular_strength) * specular;
}

vec3
apply_lighting (vec3 p, vec3 normal, vec3 color, vec3 camera_pos)
{
  return apply_lighting (p, normal, color, camera_pos,
                         get_default_lighting ());
}

#endif
