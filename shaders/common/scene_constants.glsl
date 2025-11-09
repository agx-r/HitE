#ifndef SCENE_CONSTANTS_GLSL
#define SCENE_CONSTANTS_GLSL

const bool SCENE_HARD_SHADOWS_ENABLED = true;
const bool SCENE_REFLECTION_ENABLED = false;
const bool SCENE_FADE_ENABLED = true;
const bool SCENE_NOISE_ENABLED = true;
const bool SCENE_NEAR_LIGHT_ENABLED = false;
const bool SCENE_AO_ENABLED = true;

const float SCENE_MAX_DISTANCE = 256.0;
const float SCENE_RAYMARCH_HIT_EPSILON = 0.02;
const float SCENE_RAYMARCH_HIT_EPSILON_FAR = 0.04;
const float SCENE_EPSILON_DISTANCE_START = 0.0;
const float SCENE_EPSILON_DISTANCE_FULL = SCENE_MAX_DISTANCE;
const int SCENE_MAX_STEPS = 128;

const float SCENE_SHADOW_EPSILON = 0.03;
const float SCENE_SHADOW_EPSILON_FAR = 0.05;
const int SCENE_MAX_SHADOW_STEPS = 27;

const float SCENE_FADE_START_FACTOR = 0.7;
const float SCENE_FADE_END_FACTOR = 1.0;

const float SCENE_NOISE_MAX_DISTANCE = 20.0;
const float SCENE_NOISE_MAX_STRENGTH = 0.05;
const float SCENE_NOISE_SHADOW_MIN_BLEND = 0.1;
const float SCENE_NOISE_SHADOW_MAX_BLEND = 1.0;
const float SCENE_NOISE_SCALE = 10.0;

const float SCENE_NEAR_LIGHT_DISTANCE = 0.0;
const float SCENE_NEAR_LIGHT_STRENGTH = 0.1;
const float SCENE_NEAR_LIGHT_POWER = 1.1;

const float SCENE_WHITE_SURFACE_LUMINANCE = 1.0;
const float SCENE_WHITE_SURFACE_DEPTH_MAX = 20.0;
const float SCENE_WHITE_SURFACE_DEPTH_REDUCE = 0.15;

const float SCENE_SHADOW_MIN_HARD = 0.5;
const float SCENE_SHADOW_SOFT_BLEND = 0.6;
const float SCENE_SHADOW_SOFT_POWER = 0.3;

const float SCENE_FOV_ASPECT_CORRECTION = 1.0;
const float SCENE_DEPTH_VALID_MIN = 0.0;

const float SCENE_NORMAL_UNPACK_SCALE = 2.0;
const float SCENE_NORMAL_UNPACK_BIAS = 1.0;
const float SCENE_HIT_FLAG_THRESHOLD = 0.5;

const vec3 SCENE_LUMINANCE_WEIGHTS = vec3 (0.299, 0.587, 0.114);
const float SCENE_SRGB_GAMMA = 1.0 / 3.2;

const float SCENE_REFLECTION_STRENGTH = 0.45;
const float SCENE_REFLECTION_FRESNEL_POWER = 4.0;
const float SCENE_REFLECTION_MAX_DISTANCE = 64.0;
const int SCENE_REFLECTION_MAX_STEPS = 32;
const float SCENE_REFLECTION_SURFACE_BIAS = 0.05;
const float SCENE_REFLECTION_MIN_STEP = 0.05;
const float SCENE_REFLECTION_DISTANCE_ATTENUATION = 80.0;
const float SCENE_REFLECTION_EPSILON_SCALE = 1.0;
const vec3 SCENE_REFLECTION_TINT = vec3 (1.0, 1.0, 1.0);

const float SCENE_AO_STRENGTH = 0.4;
const float SCENE_AO_BIAS = 0.04;
const float SCENE_AO_STEP = 0.4;
const float SCENE_AO_DISTANCE = 4.0;
const float SCENE_AO_SCALE_DECAY = 0.6;
const int SCENE_AO_SAMPLES = 6;

float
scene_epsilon_distance_factor (float distance_to_camera)
{
  float range = SCENE_EPSILON_DISTANCE_FULL - SCENE_EPSILON_DISTANCE_START;
  if (range <= 0.0)
    {
      return (distance_to_camera >= SCENE_EPSILON_DISTANCE_FULL) ? 1.0 : 0.0;
    }

  float t = (distance_to_camera - SCENE_EPSILON_DISTANCE_START) / range;
  return clamp (t, 0.0, 1.0);
}

float
scene_raymarch_epsilon (float distance_to_camera)
{
  float factor = scene_epsilon_distance_factor (distance_to_camera);
  return mix (SCENE_RAYMARCH_HIT_EPSILON, SCENE_RAYMARCH_HIT_EPSILON_FAR,
              factor);
}

float
scene_shadow_epsilon (float distance_to_camera)
{
  float factor = scene_epsilon_distance_factor (distance_to_camera);
  return mix (SCENE_SHADOW_EPSILON, SCENE_SHADOW_EPSILON_FAR, factor);
}

#endif
