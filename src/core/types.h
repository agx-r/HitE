#ifndef HITE_TYPES_H
#define HITE_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t entity_id_t;
typedef uint32_t component_id_t;
typedef uint64_t resource_id_t;

#define INVALID_ENTITY 0xFFFFFFFF
#define MAX_ENTITIES 65536
#define MAX_COMPONENT_TYPES 256

#define ALIGN_16 __attribute__ ((aligned (16)))
#define ALIGN_32 __attribute__ ((aligned (32)))
#define ALIGN_64 __attribute__ ((aligned (64)))

typedef struct
{
  float x, y, z, w;
} ALIGN_16 vec4_t;

typedef struct
{
  float x, y, z;
  float _padding;
} ALIGN_16 vec3_t;

typedef struct
{
  float x, y;
  float _padding[2];
} ALIGN_16 vec2_t;

typedef struct
{
  vec4_t rows[4];
} ALIGN_64 mat4_t;

typedef struct
{
  vec4_t rotation;
  vec3_t position;
  vec3_t scale;
} ALIGN_64 transform_t;

typedef struct
{
  double current_time;
  float delta_time;
  float fixed_delta_time;
  uint64_t frame_count;
} time_info_t;

typedef enum
{
  RESULT_OK = 0,
  RESULT_ERROR_ALLOCATION,
  RESULT_ERROR_INVALID_PARAMETER,
  RESULT_ERROR_NOT_FOUND,
  RESULT_ERROR_DEPENDENCY_MISSING,
  RESULT_ERROR_VULKAN,
  RESULT_ERROR_SHADER_COMPILATION,
  RESULT_ERROR_FILE_NOT_FOUND,
} result_code_t;

typedef struct
{
  result_code_t code;
  const char *message;
} result_t;

#define RESULT_SUCCESS ((result_t){ RESULT_OK, NULL })
#define RESULT_ERROR(code, msg) ((result_t){ code, msg })

#endif
