#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_COMPONENT_CAPACITY 1024

// Helper: Aligned allocation
static void *
aligned_alloc_wrapper (size_t alignment, size_t size)
{
  void *ptr = NULL;
  if (posix_memalign (&ptr, alignment, size) != 0)
    {
      return NULL;
    }
  return ptr;
}

// World creation
ecs_world_t *
ecs_world_create (void)
{
  ecs_world_t *world = calloc (1, sizeof (ecs_world_t));
  if (!world)
    return NULL;

  world->entity_versions = calloc (MAX_ENTITIES, sizeof (entity_id_t));
  world->free_entities = calloc (MAX_ENTITIES, sizeof (entity_id_t));
  world->component_arrays
      = calloc (MAX_COMPONENT_TYPES, sizeof (component_array_t));
  world->component_lookup.names
      = calloc (MAX_COMPONENT_TYPES, sizeof (char *));
  world->component_lookup.ids
      = calloc (MAX_COMPONENT_TYPES, sizeof (component_id_t));

  if (!world->entity_versions || !world->free_entities
      || !world->component_arrays || !world->component_lookup.names)
    {
      ecs_world_destroy (world);
      return NULL;
    }

  world->next_entity_id = 1;
  world->time.fixed_delta_time = 1.0f / 60.0f;

  return world;
}

void
ecs_world_destroy (ecs_world_t *world)
{
  if (!world)
    return;

  // Cleanup component arrays
  for (size_t i = 0; i < world->component_count; i++)
    {
      component_array_t *array = &world->component_arrays[i];
      if (array->descriptor.destroy)
        {
          for (size_t j = 0; j < array->count; j++)
            {
              void *data
                  = (char *)array->data + j * array->descriptor.data_size;
              array->descriptor.destroy (data);
            }
        }
      free (array->data);
      free (array->entities);
      free (array->active);
    }

  free (world->component_arrays);
  free (world->entity_versions);
  free (world->free_entities);
  free (world->component_lookup.names);
  free (world->component_lookup.ids);
  free (world);
}

// Component registration
result_t
ecs_register_component (ecs_world_t *world,
                        const component_descriptor_t *descriptor,
                        component_id_t *out_id)
{
  if (!world || !descriptor || !descriptor->name)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

  if (world->component_count >= MAX_COMPONENT_TYPES)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Max component types reached");
    }

  // Check for duplicates
  for (size_t i = 0; i < world->component_count; i++)
    {
      if (strcmp (world->component_arrays[i].descriptor.name, descriptor->name)
          == 0)
        {
          return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                               "Component already registered");
        }
    }

  component_id_t id = world->component_count++;
  component_array_t *array = &world->component_arrays[id];

  array->descriptor = *descriptor;
  array->id = id;
  array->capacity = INITIAL_COMPONENT_CAPACITY;

  size_t alignment = descriptor->alignment > 0 ? descriptor->alignment : 16;
  array->data = aligned_alloc_wrapper (alignment, descriptor->data_size
                                                      * array->capacity);
  array->entities = calloc (array->capacity, sizeof (entity_id_t));
  array->active = calloc (array->capacity, sizeof (bool));

  if (!array->data || !array->entities || !array->active)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                           "Failed to allocate component storage");
    }

  // Add to lookup
  world->component_lookup.names[id] = descriptor->name;
  world->component_lookup.ids[id] = id;
  world->component_lookup.count++;

  if (out_id)
    *out_id = id;
  return RESULT_SUCCESS;
}

component_id_t
ecs_get_component_id (const ecs_world_t *world, const char *name)
{
  if (!world || !name)
    return INVALID_ENTITY;

  for (size_t i = 0; i < world->component_lookup.count; i++)
    {
      if (strcmp (world->component_lookup.names[i], name) == 0)
        {
          return world->component_lookup.ids[i];
        }
    }
  return INVALID_ENTITY;
}

// Entity management
entity_id_t
ecs_entity_create (ecs_world_t *world)
{
  if (!world)
    return INVALID_ENTITY;

  entity_id_t id;
  if (world->free_entity_count > 0)
    {
      id = world->free_entities[--world->free_entity_count];
    }
  else
    {
      id = world->next_entity_id++;
      if (id >= MAX_ENTITIES)
        return INVALID_ENTITY;
    }

  world->entity_versions[id]++;
  return id;
}

void
ecs_entity_destroy (ecs_world_t *world, entity_id_t entity)
{
  if (!world || !ecs_entity_is_valid (world, entity))
    return;

  // Remove all components
  for (size_t i = 0; i < world->component_count; i++)
    {
      ecs_remove_component (world, entity, i);
    }

  world->free_entities[world->free_entity_count++] = entity;
}

bool
ecs_entity_is_valid (const ecs_world_t *world, entity_id_t entity)
{
  return world && entity < MAX_ENTITIES && world->entity_versions[entity] > 0;
}

// Component operations
result_t
ecs_add_component (ecs_world_t *world, entity_id_t entity,
                   component_id_t component_id, const void *initial_data)
{
  if (!world || !ecs_entity_is_valid (world, entity)
      || component_id >= world->component_count)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

  component_array_t *array = &world->component_arrays[component_id];

  // Check if already has component
  for (size_t i = 0; i < array->count; i++)
    {
      if (array->entities[i] == entity && array->active[i])
        {
          return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                               "Entity already has component");
        }
    }

  // Check dependencies
  if (array->descriptor.dependencies)
    {
      for (size_t i = 0; array->descriptor.dependencies[i] != NULL; i++)
        {
          component_id_t dep_id = ecs_get_component_id (
              world, array->descriptor.dependencies[i]);
          if (dep_id == INVALID_ENTITY
              || !ecs_has_component (world, entity, dep_id))
            {
              char msg[256];
              snprintf (msg, sizeof (msg), "Missing dependency: %s",
                        array->descriptor.dependencies[i]);
              return RESULT_ERROR (RESULT_ERROR_DEPENDENCY_MISSING, msg);
            }
        }
    }

  // Grow if needed
  if (array->count >= array->capacity)
    {
      size_t new_capacity = array->capacity * 2;
      size_t alignment
          = array->descriptor.alignment > 0 ? array->descriptor.alignment : 16;

      void *new_data = aligned_alloc_wrapper (
          alignment, array->descriptor.data_size * new_capacity);
      entity_id_t *new_entities
          = realloc (array->entities, new_capacity * sizeof (entity_id_t));
      bool *new_active = realloc (array->active, new_capacity * sizeof (bool));

      if (!new_data || !new_entities || !new_active)
        {
          free (new_data);
          return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                               "Failed to grow component array");
        }

      memcpy (new_data, array->data,
              array->descriptor.data_size * array->count);
      free (array->data);

      array->data = new_data;
      array->entities = new_entities;
      array->active = new_active;
      array->capacity = new_capacity;
    }

  // Add component
  size_t index = array->count++;
  array->entities[index] = entity;
  array->active[index] = true;

  void *component_data
      = (char *)array->data + index * array->descriptor.data_size;
  if (initial_data)
    {
      memcpy (component_data, initial_data, array->descriptor.data_size);
    }
  else
    {
      memset (component_data, 0, array->descriptor.data_size);
    }

  // Call start
  if (array->descriptor.start)
    {
      result_t result
          = array->descriptor.start (world, entity, component_data);
      if (result.code != RESULT_OK)
        {
          array->count--;
          return result;
        }
    }

  return RESULT_SUCCESS;
}

result_t
ecs_remove_component (ecs_world_t *world, entity_id_t entity,
                      component_id_t component_id)
{
  if (!world || component_id >= world->component_count)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

  component_array_t *array = &world->component_arrays[component_id];

  for (size_t i = 0; i < array->count; i++)
    {
      if (array->entities[i] == entity && array->active[i])
        {
          void *data = (char *)array->data + i * array->descriptor.data_size;

          if (array->descriptor.destroy)
            {
              array->descriptor.destroy (data);
            }

          array->active[i] = false;
          return RESULT_SUCCESS;
        }
    }

  return RESULT_ERROR (RESULT_ERROR_NOT_FOUND, "Component not found");
}

void *
ecs_get_component (ecs_world_t *world, entity_id_t entity,
                   component_id_t component_id)
{
  if (!world || component_id >= world->component_count)
    return NULL;

  component_array_t *array = &world->component_arrays[component_id];

  for (size_t i = 0; i < array->count; i++)
    {
      if (array->entities[i] == entity && array->active[i])
        {
          return (char *)array->data + i * array->descriptor.data_size;
        }
    }

  return NULL;
}

bool
ecs_has_component (const ecs_world_t *world, entity_id_t entity,
                   component_id_t component_id)
{
  if (!world || component_id >= world->component_count)
    return false;

  const component_array_t *array = &world->component_arrays[component_id];

  for (size_t i = 0; i < array->count; i++)
    {
      if (array->entities[i] == entity && array->active[i])
        {
          return true;
        }
    }

  return false;
}

// System execution
result_t
ecs_system_start (ecs_world_t *world)
{
  if (!world)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid world");

  for (size_t i = 0; i < world->component_count; i++)
    {
      component_array_t *array = &world->component_arrays[i];
      if (!array->descriptor.start)
        continue;

      for (size_t j = 0; j < array->count; j++)
        {
          if (!array->active[j])
            continue;

          void *data = (char *)array->data + j * array->descriptor.data_size;
          result_t result
              = array->descriptor.start (world, array->entities[j], data);
          if (result.code != RESULT_OK)
            return result;
        }
    }

  return RESULT_SUCCESS;
}

result_t
ecs_system_update (ecs_world_t *world)
{
  if (!world)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid world");

  for (size_t i = 0; i < world->component_count; i++)
    {
      component_array_t *array = &world->component_arrays[i];
      if (!array->descriptor.update)
        continue;

      for (size_t j = 0; j < array->count; j++)
        {
          if (!array->active[j])
            continue;

          void *data = (char *)array->data + j * array->descriptor.data_size;
          result_t result = array->descriptor.update (
              world, array->entities[j], data, &world->time);
          if (result.code != RESULT_OK)
            return result;
        }
    }

  return RESULT_SUCCESS;
}

result_t
ecs_system_render (ecs_world_t *world)
{
  if (!world)
    return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "Invalid world");

  for (size_t i = 0; i < world->component_count; i++)
    {
      component_array_t *array = &world->component_arrays[i];
      if (!array->descriptor.render)
        continue;

      for (size_t j = 0; j < array->count; j++)
        {
          if (!array->active[j])
            continue;

          const void *data
              = (const char *)array->data + j * array->descriptor.data_size;
          result_t result
              = array->descriptor.render (world, array->entities[j], data);
          if (result.code != RESULT_OK)
            return result;
        }
    }

  return RESULT_SUCCESS;
}

void
ecs_iterate_components (ecs_world_t *world, component_id_t component_id,
                        ecs_iterate_fn callback, void *user_data)
{
  if (!world || component_id >= world->component_count || !callback)
    return;

  component_array_t *array = &world->component_arrays[component_id];

  for (size_t i = 0; i < array->count; i++)
    {
      if (!array->active[i])
        continue;

      void *data = (char *)array->data + i * array->descriptor.data_size;
      callback (world, array->entities[i], data, user_data);
    }
}
