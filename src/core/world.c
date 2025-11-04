#include "world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

world_manager_t *
world_manager_create (void)
{
  world_manager_t *manager = calloc (1, sizeof (world_manager_t));
  if (!manager)
    return NULL;

  manager->loaded_worlds = calloc (32, sizeof (world_definition_t *));
  if (!manager->loaded_worlds)
    {
      free (manager);
      return NULL;
    }

  return manager;
}

void
world_manager_destroy (world_manager_t *manager)
{
  if (!manager)
    return;

  if (manager->active_world)
    {
      ecs_world_destroy (manager->active_world);
    }

  free (manager->loaded_worlds);
  free (manager);
}

result_t
world_load (world_manager_t *manager, const world_definition_t *definition)
{
  if (!manager || !definition)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

  // Use existing world if available, otherwise create new one
  ecs_world_t *world = manager->active_world;
  if (!world)
    {
      world = ecs_world_create ();
      if (!world)
        {
          return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                               "Failed to create ECS world");
        }
    }

  // Set time configuration
  world->time.fixed_delta_time = definition->fixed_delta_time;

  // Run procedural generator if provided
  if (definition->generator)
    {
      result_t result
          = definition->generator (world, definition->generator_data);
      if (result.code != RESULT_OK)
        {
          ecs_world_destroy (world);
          return result;
        }
    }

  // Instantiate entity templates
  if (definition->entity_templates && definition->entity_template_count > 0)
    {
      result_t result
          = world_instantiate_templates (world, definition->entity_templates,
                                         definition->entity_template_count);
      if (result.code != RESULT_OK)
        {
          ecs_world_destroy (world);
          return result;
        }
    }

  // Set active world (don't destroy if we're reusing it)
  if (manager->active_world != world && manager->active_world)
    {
      ecs_world_destroy (manager->active_world);
    }

  manager->active_world = world;
  manager->current_definition = (world_definition_t *)definition;

  // Call start on all components
  result_t result = ecs_system_start (world);
  if (result.code != RESULT_OK)
    {
      return result;
    }

  return RESULT_SUCCESS;
}

result_t
world_instantiate_templates (ecs_world_t *world,
                             const entity_template_t *templates, size_t count)
{
  if (!world || !templates)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

  for (size_t i = 0; i < count; i++)
    {
      const entity_template_t *tmpl = &templates[i];

      // Create entity
      entity_id_t entity = ecs_entity_create (world);
      if (entity == INVALID_ENTITY)
        {
          return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                               "Failed to create entity");
        }

      // Add components
      for (size_t j = 0; j < tmpl->component_count; j++)
        {
          const char *component_name = tmpl->components[j].component_name;
          const void *initial_data = tmpl->components[j].initial_data;

          component_id_t component_id
              = ecs_get_component_id (world, component_name);
          if (component_id == INVALID_ENTITY)
            {
              char msg[256];
              snprintf (msg, sizeof (msg), "Component not found: %s",
                        component_name);
              return RESULT_ERROR (RESULT_ERROR_NOT_FOUND, msg);
            }

          result_t result
              = ecs_add_component (world, entity, component_id, initial_data);
          if (result.code != RESULT_OK)
            {
              return result;
            }
        }
    }

  return RESULT_SUCCESS;
}

result_t
world_switch (world_manager_t *manager, const world_definition_t *definition)
{
  // Same as load for now, but could add transition logic
  return world_load (manager, definition);
}

result_t
world_update (world_manager_t *manager, float delta_time)
{
  if (!manager || !manager->active_world)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "No active world");
    }

  ecs_world_t *world = manager->active_world;

  // Update time
  world->time.delta_time = delta_time;
  world->time.current_time += delta_time;
  world->time.frame_count++;

  // Update systems
  return ecs_system_update (world);
}
