#ifndef HITE_GUI_COMPONENT_H
#define HITE_GUI_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

// Maximum number of text elements that can be displayed
#define GUI_MAX_TEXT_ELEMENTS 32

// Text element for GUI rendering
typedef struct
{
  char text[256]; // Text to display
  float x, y;     // Position (normalized coordinates 0-1)
  float size;     // Text size multiplier
  vec4_t color;   // Text color (RGBA)
  bool active;    // Whether this element is active
} gui_text_element_t;

// GUI component data - helper for rendering UI
// Depends on camera component
typedef struct
{
  // Camera reference (required dependency)
  entity_id_t camera_entity;

  // Text elements to render
  gui_text_element_t text_elements[GUI_MAX_TEXT_ELEMENTS];
  size_t text_element_count;

  // GUI state
  bool enabled;
} ALIGN_64 gui_component_t;

// Component lifecycle
result_t gui_component_start (ecs_world_t *world, entity_id_t entity,
                              void *component_data);
result_t gui_component_update (ecs_world_t *world, entity_id_t entity,
                               void *component_data, const time_info_t *time);
result_t gui_component_render (ecs_world_t *world, entity_id_t entity,
                               const void *component_data);
void gui_component_destroy (void *component_data);

// Register component
void gui_component_register (ecs_world_t *world);

// GUI helper functions
// Add text element to GUI
result_t gui_add_text (gui_component_t *gui, const char *text, float x,
                       float y, float size, vec4_t color);

// Update existing text element
result_t gui_update_text (gui_component_t *gui, size_t index,
                          const char *text);

// Clear all text elements
void gui_clear_text (gui_component_t *gui);

#endif // HITE_GUI_COMPONENT_H
