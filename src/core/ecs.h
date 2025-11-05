#ifndef HITE_ECS_H
#define HITE_ECS_H

#include "types.h"

typedef struct ecs_world_t ecs_world_t;

typedef result_t (*component_start_fn) (ecs_world_t *world, entity_id_t entity,
                                        void *component_data);
typedef result_t (*component_update_fn) (ecs_world_t *world,
                                         entity_id_t entity,
                                         void *component_data,
                                         const time_info_t *time);
typedef result_t (*component_render_fn) (ecs_world_t *world,
                                         entity_id_t entity,
                                         const void *component_data);
typedef void (*component_destroy_fn) (void *component_data);

typedef struct
{
  const char *name;
  size_t data_size;
  size_t alignment;

  const char **dependencies;

  component_start_fn start;
  component_update_fn update;
  component_render_fn render;
  component_destroy_fn destroy;
} component_descriptor_t;

typedef struct
{
  component_descriptor_t descriptor;
  component_id_t id;

  void *data;
  entity_id_t *entities;
  bool *active;

  size_t count;
  size_t capacity;
} component_array_t;

typedef struct
{
  entity_id_t *sparse;
  entity_id_t *dense;
  size_t count;
  size_t capacity;
} sparse_set_t;

struct ecs_world_t
{

  component_array_t *component_arrays;
  size_t component_count;

  entity_id_t *entity_versions;
  entity_id_t next_entity_id;
  entity_id_t *free_entities;
  size_t free_entity_count;

  time_info_t time;

  struct
  {
    const char **names;
    component_id_t *ids;
    size_t count;
  } component_lookup;
};

ecs_world_t *ecs_world_create (void);
void ecs_world_destroy (ecs_world_t *world);

result_t ecs_register_component (ecs_world_t *world,
                                 const component_descriptor_t *descriptor,
                                 component_id_t *out_id);
component_id_t ecs_get_component_id (const ecs_world_t *world,
                                     const char *name);

entity_id_t ecs_entity_create (ecs_world_t *world);
void ecs_entity_destroy (ecs_world_t *world, entity_id_t entity);
bool ecs_entity_is_valid (const ecs_world_t *world, entity_id_t entity);

result_t ecs_add_component (ecs_world_t *world, entity_id_t entity,
                            component_id_t component_id,
                            const void *initial_data);
result_t ecs_remove_component (ecs_world_t *world, entity_id_t entity,
                               component_id_t component_id);
void *ecs_get_component (ecs_world_t *world, entity_id_t entity,
                         component_id_t component_id);
bool ecs_has_component (const ecs_world_t *world, entity_id_t entity,
                        component_id_t component_id);

result_t ecs_system_start (ecs_world_t *world);
result_t ecs_system_update (ecs_world_t *world);
result_t ecs_system_render (ecs_world_t *world);

typedef void (*ecs_iterate_fn) (ecs_world_t *world, entity_id_t entity,
                                void *component_data, void *user_data);
void ecs_iterate_components (ecs_world_t *world, component_id_t component_id,
                             ecs_iterate_fn callback, void *user_data);

#endif
