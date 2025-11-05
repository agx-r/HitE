#include "gui_component.h"
#include "camera_component.h"

#include <stdio.h>
#include <string.h>

// THIS COMPONENT IS NOT READY ! TODO !

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

  // Verify camera is still valid
  if (!ecs_entity_is_valid (world, gui->camera_entity))
    {
      // Try to find a new active camera
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

  // TODO the rendering will be here
  // TODO for now just printf

  return RESULT_SUCCESS;
}

void
gui_component_destroy (void *component_data)
{
  (void)component_data;
  // No dynamic allocations to clean up
}

// Register component
void
gui_component_register (ecs_world_t *world)
{
  // Define dependencies: GUI depends on camera
  static const char *dependencies[] = { "camera", NULL };

  component_descriptor_t desc = { 0 };
  desc.name = "gui";
  desc.data_size = sizeof (gui_component_t);
  desc.alignment = 64;
  desc.dependencies = dependencies;
  desc.start = gui_component_start;
  desc.update = gui_component_update;
  desc.render = gui_component_render;
  desc.destroy = gui_component_destroy;

  component_id_t id;
  result_t result = ecs_register_component (world, &desc, &id);

  if (result.code == RESULT_OK)
    {
      printf ("[GUI] GUI component registered (ID: %u)\n", id);
    }
  else
    {
      printf ("[GUI] Failed to register GUI component: %s\n", result.message);
    }
}

// helpers
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
