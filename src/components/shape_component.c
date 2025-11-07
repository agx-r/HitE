#include "shape_component.h"
#include "component_registry.h"
#include <string.h>

result_t
shape_component_start (ecs_world_t *world, entity_id_t entity,
                       void *component_data)
{
  (void)world;
  (void)entity;

  shape_component_t *shape = (shape_component_t *)component_data;

  if (shape->color.w == 0.0f)
    {
      shape->color = (vec4_t){ .x = 0.8f, .y = 0.8f, .z = 0.8f, .w = 1.0f };
    }

  shape->dirty = true;
  shape->visible = true;

  return RESULT_SUCCESS;
}

result_t
shape_component_update (ecs_world_t *world, entity_id_t entity,
                        void *component_data, const time_info_t *time)
{
  (void)world;
  (void)entity;
  (void)time;
  (void)component_data;
  return RESULT_SUCCESS;
}

result_t
shape_component_render (ecs_world_t *world, entity_id_t entity,
                        const void *component_data)
{
  (void)world;
  (void)entity;
  (void)component_data;
  return RESULT_SUCCESS;
}

void
shape_component_destroy (void *component_data)
{
  (void)component_data;
}

void
shape_component_register (ecs_world_t *world)
{
  REGISTER_COMPONENT (world, "shape", shape_component_t, shape_component_start,
                      shape_component_update, shape_component_render,
                      shape_component_destroy, "Shape", 0, NULL);
}
