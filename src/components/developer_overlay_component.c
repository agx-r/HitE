#include "developer_overlay_component.h"
#include "camera_component.h"
#include "component_registry.h"
#include "gui_component.h"

#include <stdio.h>
#include <string.h>

static gui_component_t *
find_gui_component (ecs_world_t *world, entity_id_t *out_entity)
{
  if (!world)
    return NULL;

  component_id_t gui_id = ecs_get_component_id (world, "gui");
  if (gui_id == INVALID_ENTITY)
    return NULL;

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

result_t
developer_overlay_component_start (ecs_world_t *world, entity_id_t entity,
                                   void *component_data)
{
  (void)entity;

  developer_overlay_component_t *overlay
      = (developer_overlay_component_t *)component_data;

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
  overlay->fps_update_interval = 0.5f;
  overlay->fps_text_initialized = false;
  overlay->enabled = true;

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

  if (!ecs_entity_is_valid (world, overlay->gui_entity))
    {

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

  overlay->frame_count++;
  overlay->frame_time_accumulator += time->delta_time;

  if (overlay->frame_time_accumulator >= overlay->fps_update_interval)
    {
      overlay->current_fps
          = overlay->frame_count / overlay->frame_time_accumulator;

      component_id_t gui_id = ecs_get_component_id (world, "gui");
      gui_component_t *gui = (gui_component_t *)ecs_get_component (
          world, overlay->gui_entity, gui_id);

      if (gui && overlay->fps_text_initialized)
        {
          char fps_text[64];
          snprintf (fps_text, sizeof (fps_text), "FPS: %.1f",
                    overlay->current_fps);
          gui_update_text (gui, overlay->fps_text_index, fps_text);

          printf ("[DevOverlay] FPS: %.1f (Frame time: %.3f ms)\n",
                  overlay->current_fps,
                  (overlay->frame_time_accumulator / overlay->frame_count)
                      * 1000.0f);
        }

      entity_id_t camera_entity = INVALID_ENTITY;
      camera_component_t *camera = camera_find_active (world, &camera_entity);
      if (camera)
        {
          printf ("[DevOverlay] Camera: (%.2f, %.2f, %.2f)\n",
                  camera->position.x, camera->position.y, camera->position.z);

          if (gui && overlay->camera_pos_text_initialized)
            {
              char camera_text[128];
              snprintf (camera_text, sizeof (camera_text),
                        "Camera: (%.2f, %.2f, %.2f)", camera->position.x,
                        camera->position.y, camera->position.z);
              gui_update_text (gui, overlay->camera_pos_text_index,
                               camera_text);
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
  static const char *dependencies[] = { "gui", NULL };
  REGISTER_COMPONENT (
      world, "developer_overlay", developer_overlay_component_t,
      developer_overlay_component_start, developer_overlay_component_update,
      developer_overlay_component_render, developer_overlay_component_destroy,
      "DevOverlay", 64, dependencies);
}
