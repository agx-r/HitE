#ifndef HITE_DEVELOPER_OVERLAY_COMPONENT_H
#define HITE_DEVELOPER_OVERLAY_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

#define DEVELOPER_OVERLAY_MAX_TEXT_ELEMENTS 32

typedef struct
{
  char text[256];
  float x, y;
  float size;
  vec4_t color;
  bool active;
} developer_overlay_text_element_t;

typedef struct
{

  entity_id_t camera_entity;

  developer_overlay_text_element_t
      text_elements[DEVELOPER_OVERLAY_MAX_TEXT_ELEMENTS];
  size_t text_element_count;

  float frame_time_accumulator;
  uint32_t frame_count;
  float current_fps;
  float fps_update_interval;

  size_t fps_text_index;
  bool fps_text_initialized;
  size_t camera_pos_text_index;
  bool camera_pos_text_initialized;

  bool enabled;
} ALIGN_64 developer_overlay_component_t;

void developer_overlay_component_register (ecs_world_t *world);

result_t developer_overlay_update_text (developer_overlay_component_t *overlay,
                                        size_t index, const char *text);

#endif
