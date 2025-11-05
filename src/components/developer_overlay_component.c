#include "developer_overlay_component.h"
#include "gui_component.h"
#include "component_registry.h"

#include <stdio.h>
#include <string.h>

// Helper function to find GUI component in the world
static gui_component_t *
find_gui_component (ecs_world_t *world, entity_id_t *out_entity)
{
  if (!world)
    return NULL;

  component_id_t gui_id = ecs_get_component_id (world, "gui");
  if (gui_id == INVALID_ENTITY)
    return NULL;

  // Find component array
  component_array_t *array = NULL;
  for (size_t i = 0; i < world->component_count; i++)
    {
      if (world->component_arrays[i].id == gui_id)
        {
          array = &world->component_arrays[i];
          break;
        }
    }

  if (!array || array->count == 0)
    return NULL;

  // Return first active GUI component
  for (size_t i = 0; i < array->count; i++)
    {
      if (array->active[i])
        {
          if (out_entity)
            *out_entity = array->entities[i];
          return (gui_component_t *)((char *)array->data
                                     + i * array->descriptor.data_size);
        }
    }

  return NULL;
}

// Component lifecycle
result_t
developer_overlay_component_start (ecs_world_t *world, entity_id_t entity,
                                   void *component_data)
{
  (void)entity;

  developer_overlay_component_t *overlay
      = (developer_overlay_component_t *)component_data;

  // Find GUI component
  entity_id_t gui_entity = INVALID_ENTITY;
  gui_component_t *gui = find_gui_component (world, &gui_entity);

  if (!gui)
    {
      return RESULT_ERROR (RESULT_ERROR_DEPENDENCY_MISSING,
                           "Developer overlay requires a GUI component");
    }

  overlay->gui_entity = gui_entity;
  overlay->frame_time_accumulator = 0.0f;
  overlay->frame_count = 0;
  overlay->current_fps = 0.0f;
  overlay->fps_update_interval = 0.5f; // Update FPS every 0.5 seconds
  overlay->fps_text_initialized = false;
  overlay->enabled = true;

  // Add FPS text element to GUI
  vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };
  result_t result = gui_add_text (gui, "FPS: --", 0.02f, 0.02f, 1.0f, white);

  if (result.code == RESULT_OK)
    {
      overlay->fps_text_index = gui->text_element_count - 1;
      overlay->fps_text_initialized = true;
    }
  else
    {
      printf ("[DevOverlay] Warning: Failed to add FPS text to GUI: %s\n",
              result.message);
    }

  printf ("[DevOverlay] Developer overlay started for entity %u (GUI: %u)\n",
          entity, gui_entity);

  return RESULT_SUCCESS;
}

result_t
developer_overlay_component_update (ecs_world_t *world, entity_id_t entity,
                                    void *component_data,
                                    const time_info_t *time)
{
  (void)entity;

  developer_overlay_component_t *overlay
      = (developer_overlay_component_t *)component_data;

  if (!overlay->enabled)
    return RESULT_SUCCESS;

  // Verify GUI entity is still valid
  if (!ecs_entity_is_valid (world, overlay->gui_entity))
    {
      // Try to find a new GUI component
      entity_id_t gui_entity = INVALID_ENTITY;
      gui_component_t *gui = find_gui_component (world, &gui_entity);

      if (gui)
        {
          overlay->gui_entity = gui_entity;
          overlay->fps_text_initialized = false;
        }
      else
        {
          overlay->enabled = false;
          return RESULT_ERROR (RESULT_ERROR_DEPENDENCY_MISSING,
                               "Developer overlay lost GUI reference");
        }
    }

  // Update FPS tracking
  overlay->frame_count++;
  overlay->frame_time_accumulator += time->delta_time;

  // Update FPS display at regular intervals
  if (overlay->frame_time_accumulator >= overlay->fps_update_interval)
    {
      overlay->current_fps
          = overlay->frame_count / overlay->frame_time_accumulator;

      // Get GUI component and update text
      component_id_t gui_id = ecs_get_component_id (world, "gui");
      gui_component_t *gui = (gui_component_t *)ecs_get_component (
          world, overlay->gui_entity, gui_id);

      if (gui && overlay->fps_text_initialized)
        {
          char fps_text[64];
          snprintf (fps_text, sizeof (fps_text), "FPS: %.1f",
                    overlay->current_fps);
          gui_update_text (gui, overlay->fps_text_index, fps_text);

          // Debug: print FPS to console
          printf ("[DevOverlay] FPS: %.1f (Frame time: %.3f ms)\n",
                  overlay->current_fps,
                  (overlay->frame_time_accumulator / overlay->frame_count)
                      * 1000.0f);
        }

      // Reset counters
      overlay->frame_time_accumulator = 0.0f;
      overlay->frame_count = 0;
    }

  return RESULT_SUCCESS;
}

result_t
developer_overlay_component_render (ecs_world_t *world, entity_id_t entity,
                                    const void *component_data)
{
  (void)world;
  (void)entity;

  const developer_overlay_component_t *overlay
      = (const developer_overlay_component_t *)component_data;

  if (!overlay->enabled)
    return RESULT_SUCCESS;

  // Actual rendering is handled by GUI component
  // This is just a placeholder for future extensions

  return RESULT_SUCCESS;
}

void
developer_overlay_component_destroy (void *component_data)
{
  (void)component_data;
  // No dynamic allocations to clean up
}

// Register component
void
developer_overlay_component_register (ecs_world_t *world)
{
  static const char *dependencies[] = { "gui", NULL };
  REGISTER_COMPONENT (world, "developer_overlay", developer_overlay_component_t,
                      developer_overlay_component_start, developer_overlay_component_update,
                      developer_overlay_component_render, developer_overlay_component_destroy,
                      "DevOverlay", 64, dependencies);
}
