#ifndef HITE_COMPONENT_REGISTRY_H
#define HITE_COMPONENT_REGISTRY_H

#include "../core/ecs.h"
#include <stdio.h>

void register_component_helper (
    ecs_world_t *world, const char *name, size_t data_size, size_t alignment,
    const char **dependencies, component_start_fn start,
    component_update_fn update, component_render_fn render,
    component_destroy_fn destroy, const char *display_name);

#define REGISTER_COMPONENT(world, name, type, start_fn, update_fn, render_fn, \
                           destroy_fn, display, align, deps)                  \
  register_component_helper (world, name, sizeof (type), align, deps,         \
                             start_fn, update_fn, render_fn, destroy_fn,      \
                             display)

void register_all_components (ecs_world_t *world);

#endif
