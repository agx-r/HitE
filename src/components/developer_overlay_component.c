#include "developer_overlay_component.h"
#include "../core/logger.h"
#include "camera_component.h"
#include "component_registry.h"

#include <stdio.h>
#include <string.h>

result_t
developer_overlay_component_start (ecs_world_t *world, entity_id_t entity,
                                   void *component_data)
{
  (void)entity;

  developer_overlay_component_t *overlay
      = (developer_overlay_component_t *)component_data;

  entity_id_t camera_entity = INVALID_ENTITY;
  camera_component_t *camera = camera_find_active (world, &camera_entity);

  if (!camera)
    {
      return RESULT_ERROR (RESULT_ERROR_DEPENDENCY_MISSING,
                           "Developer overlay requires an active camera");
    }

  overlay->camera_entity = camera_entity;
  overlay->text_element_count = 0;
  overlay->frame_time_accumulator = 0.0f;
  overlay->frame_count = 0;
  overlay->current_fps = 0.0f;
  overlay->fps_update_interval = 0.5f;
  overlay->fps_text_initialized = false;
  overlay->camera_pos_text_initialized = false;
  overlay->enabled = true;

  vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };
  result_t result = developer_overlay_add_text (overlay, "FPS: --", 0.02f,
                                                0.02f, 1.0f, white);

  if (result.code == RESULT_OK)
    {
      overlay->fps_text_index = overlay->text_element_count - 1;
      overlay->fps_text_initialized = true;
    }
  else
    {
      LOG_WARNING ("DevOverlay", "Failed to add FPS text: %s", result.message);
    }

  LOG_INFO ("DevOverlay",
            "Developer overlay started for entity %u (camera: %u)", entity,
            camera_entity);

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

  if (!ecs_entity_is_valid (world, overlay->camera_entity))
    {
      entity_id_t camera_entity = INVALID_ENTITY;
      camera_component_t *camera = camera_find_active (world, &camera_entity);

      if (camera)
        {
          overlay->camera_entity = camera_entity;
        }
      else
        {
          overlay->enabled = false;
          return RESULT_ERROR (RESULT_ERROR_DEPENDENCY_MISSING,
                               "Developer overlay lost camera reference");
        }
    }

  overlay->frame_count++;
  overlay->frame_time_accumulator += time->delta_time;

  if (overlay->frame_time_accumulator >= overlay->fps_update_interval)
    {
      overlay->current_fps
          = overlay->frame_count / overlay->frame_time_accumulator;

      if (overlay->fps_text_initialized)
        {
          char fps_text[64];
          snprintf (fps_text, sizeof (fps_text), "FPS: %.1f",
                    overlay->current_fps);
          developer_overlay_update_text (overlay, overlay->fps_text_index,
                                         fps_text);

          LOG_DEBUG ("DevOverlay", "FPS: %.1f (Frame time: %.3f ms)",
                     overlay->current_fps,
                     (overlay->frame_time_accumulator / overlay->frame_count)
                         * 1000.0f);
        }

      entity_id_t camera_entity = INVALID_ENTITY;
      camera_component_t *camera = camera_find_active (world, &camera_entity);
      if (camera)
        {
          LOG_DEBUG ("DevOverlay", "Camera: (%.2f, %.2f, %.2f)",
                     camera->position.x, camera->position.y,
                     camera->position.z);

          if (overlay->camera_pos_text_initialized)
            {
              char camera_text[128];
              snprintf (camera_text, sizeof (camera_text),
                        "Camera: (%.2f, %.2f, %.2f)", camera->position.x,
                        camera->position.y, camera->position.z);
              developer_overlay_update_text (
                  overlay, overlay->camera_pos_text_index, camera_text);
            }
        }

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

  return RESULT_SUCCESS;
}

void
developer_overlay_component_destroy (void *component_data)
{
  (void)component_data;
}

void
developer_overlay_component_register (ecs_world_t *world)
{
  static const char *dependencies[] = { "camera", NULL };
  REGISTER_COMPONENT (
      world, "developer_overlay", developer_overlay_component_t,
      developer_overlay_component_start, developer_overlay_component_update,
      developer_overlay_component_render, developer_overlay_component_destroy,
      "DevOverlay", 64, dependencies);
}

result_t
developer_overlay_add_text (developer_overlay_component_t *overlay,
                            const char *text, float x, float y, float size,
                            vec4_t color)
{
  if (!overlay || !text)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid arguments");
    }

  if (overlay->text_element_count >= DEVELOPER_OVERLAY_MAX_TEXT_ELEMENTS)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Developer overlay text element limit reached");
    }

  developer_overlay_text_element_t *element
      = &overlay->text_elements[overlay->text_element_count];
  strncpy (element->text, text, sizeof (element->text) - 1);
  element->text[sizeof (element->text) - 1] = '\0';
  element->x = x;
  element->y = y;
  element->size = size;
  element->color = color;
  element->active = true;

  overlay->text_element_count++;

  return RESULT_SUCCESS;
}

result_t
developer_overlay_update_text (developer_overlay_component_t *overlay,
                               size_t index, const char *text)
{
  if (!overlay || !text)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid arguments");
    }

  if (index >= overlay->text_element_count)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid text element index");
    }

  developer_overlay_text_element_t *element = &overlay->text_elements[index];
  strncpy (element->text, text, sizeof (element->text) - 1);
  element->text[sizeof (element->text) - 1] = '\0';

  return RESULT_SUCCESS;
}

void
developer_overlay_clear_text (developer_overlay_component_t *overlay)
{
  if (!overlay)
    return;

  overlay->text_element_count = 0;
}
