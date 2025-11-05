#include "component_registry.h"

void
register_component_helper (ecs_world_t *world, const char *name,
                           size_t data_size, size_t alignment,
                           const char **dependencies, component_start_fn start,
                           component_update_fn update,
                           component_render_fn render,
                           component_destroy_fn destroy,
                           const char *display_name)
{
  component_descriptor_t desc = { 0 };
  desc.name = name;
  desc.data_size = data_size;
  desc.alignment = alignment;
  desc.dependencies = dependencies;
  desc.start = start;
  desc.update = update;
  desc.render = render;
  desc.destroy = destroy;

  component_id_t id;
  result_t result = ecs_register_component (world, &desc, &id);

  if (result.code == RESULT_OK)
    {
      printf ("[%s] %s component registered (ID: %u)\n", display_name,
              display_name, id);
    }
  else
    {
      printf ("[%s] Failed to register %s component: %s\n", display_name,
              display_name, result.message);
    }
}
