#include "gui_component.h"
#include "camera_component.h"
#include "component_registry.h"

#include <stdio.h>
#include <string.h>

result_t
gui_component_start (ecs_world_t *world, entity_id_t entity,
                     void *component_data)
{
  (void)entity;

  gui_component_t *gui = (gui_component_t *)component_data;

  entity_id_t camera_entity = INVALID_ENTITY;
  camera_component_t *camera = camera_find_active (world, &camera_entity);

  if (!camera)
    {
      return RESULT_ERROR (RESULT_ERROR_DEPENDENCY_MISSING,
                           "GUI component requires an active camera");
    }

  gui->camera_entity = camera_entity;
  gui->text_element_count = 0;
  gui->enabled = true;

  printf ("[GUI] GUI component started for entity %u (camera: %u)\n", entity,
          camera_entity);

  return RESULT_SUCCESS;
}

result_t
gui_component_update (ecs_world_t *world, entity_id_t entity,
                      void *component_data, const time_info_t *time)
{
  (void)world;
  (void)entity;
  (void)time;

  gui_component_t *gui = (gui_component_t *)component_data;

  if (!ecs_entity_is_valid (world, gui->camera_entity))
    {

      entity_id_t camera_entity = INVALID_ENTITY;
      camera_component_t *camera = camera_find_active (world, &camera_entity);

      if (camera)
        {
          gui->camera_entity = camera_entity;
        }
      else
        {
          gui->enabled = false;
          return RESULT_ERROR (RESULT_ERROR_DEPENDENCY_MISSING,
                               "GUI component lost camera reference");
        }
    }

  return RESULT_SUCCESS;
}

result_t
gui_component_render (ecs_world_t *world, entity_id_t entity,
                      const void *component_data)
{
  (void)world;
  (void)entity;

  const gui_component_t *gui = (const gui_component_t *)component_data;

  if (!gui->enabled)
    return RESULT_SUCCESS;

  return RESULT_SUCCESS;
}

void
gui_component_destroy (void *component_data)
{
  (void)component_data;
}

void
gui_component_register (ecs_world_t *world)
{
  static const char *dependencies[] = { "camera", NULL };
  REGISTER_COMPONENT (world, "gui", gui_component_t, gui_component_start,
                      gui_component_update, gui_component_render,
                      gui_component_destroy, "GUI", 64, dependencies);
}

result_t
gui_add_text (gui_component_t *gui, const char *text, float x, float y,
              float size, vec4_t color)
{
  if (!gui || !text)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid arguments");
    }

  if (gui->text_element_count >= GUI_MAX_TEXT_ELEMENTS)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "GUI text element limit reached");
    }

  gui_text_element_t *element = &gui->text_elements[gui->text_element_count];
  strncpy (element->text, text, sizeof (element->text) - 1);
  element->text[sizeof (element->text) - 1] = '\0';
  element->x = x;
  element->y = y;
  element->size = size;
  element->color = color;
  element->active = true;

  gui->text_element_count++;

  return RESULT_SUCCESS;
}

result_t
gui_update_text (gui_component_t *gui, size_t index, const char *text)
{
  if (!gui || !text)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid arguments");
    }

  if (index >= gui->text_element_count)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid text element index");
    }

  gui_text_element_t *element = &gui->text_elements[index];
  strncpy (element->text, text, sizeof (element->text) - 1);
  element->text[sizeof (element->text) - 1] = '\0';

  return RESULT_SUCCESS;
}

void
gui_clear_text (gui_component_t *gui)
{
  if (!gui)
    return;

  gui->text_element_count = 0;
}
