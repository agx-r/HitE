#ifndef HITE_GUI_COMPONENT_H
#define HITE_GUI_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

#define GUI_MAX_TEXT_ELEMENTS 32

typedef struct
{
  char text[256];
  float x, y;
  float size;
  vec4_t color;
  bool active;
} gui_text_element_t;

typedef struct
{

  entity_id_t camera_entity;

  gui_text_element_t text_elements[GUI_MAX_TEXT_ELEMENTS];
  size_t text_element_count;

  bool enabled;
} ALIGN_64 gui_component_t;

result_t gui_component_start (ecs_world_t *world, entity_id_t entity,
                              void *component_data);
result_t gui_component_update (ecs_world_t *world, entity_id_t entity,
                               void *component_data, const time_info_t *time);
result_t gui_component_render (ecs_world_t *world, entity_id_t entity,
                               const void *component_data);
void gui_component_destroy (void *component_data);

void gui_component_register (ecs_world_t *world);

result_t gui_add_text (gui_component_t *gui, const char *text, float x,
                       float y, float size, vec4_t color);

result_t gui_update_text (gui_component_t *gui, size_t index,
                          const char *text);

void gui_clear_text (gui_component_t *gui);

#endif
