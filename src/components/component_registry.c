

#include "component_registry.h"
#include "camera_component.h"
#include "camera_movement_component.h"
#include "camera_rotation_component.h"
#include "developer_overlay_component.h"
#include "lighting_component.h"
#include "shape_component.h"

#include "../core/component_parsers.h"
#include "../core/ecs.h"
#include "../core/logger.h"

void
register_component_helper (ecs_world_t *world, const char *name,
                           size_t data_size, size_t alignment,
                           const char **dependencies, component_start_fn start,
                           component_update_fn update,
                           component_render_fn render,
                           component_destroy_fn destroy,
                           const char *display_name)
{
  if (!world || !name)
    {
      LOG_ERROR ("Component Registry", "Invalid arguments");
      return;
    }

  component_descriptor_t descriptor = { 0 };
  descriptor.name = name;
  descriptor.data_size = data_size;
  descriptor.alignment = alignment > 0 ? alignment : 16;
  descriptor.dependencies = dependencies;
  descriptor.start = start;
  descriptor.update = update;
  descriptor.render = render;
  descriptor.destroy = destroy;

  component_id_t out_id;
  result_t result = ecs_register_component (world, &descriptor, &out_id);
  if (result.code != RESULT_OK)
    {
      LOG_ERROR ("Component Registry", "Failed to register component '%s': %s",
                 name, result.message);
    }
}
