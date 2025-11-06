#include "world.h"
#include "component_parsers.h"
#include "logger.h"
#include "prefab.h"
#include "world_loader.h"
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
world_load (world_manager_t *manager, const world_definition_t *definition,
            prefab_system_t *prefab_system)
{
  if (!manager || !definition)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

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

  world->time.fixed_delta_time = definition->fixed_delta_time;

  if (definition->entity_templates && definition->entity_template_count > 0)
    {
      result_t result = world_instantiate_templates (
          world, definition->entity_templates,
          definition->entity_template_count, prefab_system);
      if (result.code != RESULT_OK)
        {
          ecs_world_destroy (world);
          return result;
        }
    }

  if (manager->active_world != world && manager->active_world)
    {
      ecs_world_destroy (manager->active_world);
    }

  manager->active_world = world;
  manager->current_definition = (world_definition_t *)definition;

  result_t result = ecs_system_start (world);
  if (result.code != RESULT_OK)
    {
      return result;
    }

  return RESULT_SUCCESS;
}

result_t
world_instantiate_templates (ecs_world_t *world,
                             const entity_template_t *templates, size_t count,
                             prefab_system_t *prefab_system)
{
  if (!world || !templates)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

  for (size_t i = 0; i < count; i++)
    {
      const entity_template_t *tmpl = &templates[i];
      entity_id_t entity = INVALID_ENTITY;

      if (tmpl->prefab_name)
        {
          if (!prefab_system)
            {
              LOG_WARNING (
                  "World",
                  "Prefab '%s' referenced but no prefab system provided",
                  tmpl->prefab_name);
              continue;
            }

          prefab_t *prefab = prefab_find (prefab_system, tmpl->prefab_name);
          if (prefab)
            {
              result_t res = prefab_instantiate (prefab, world, &entity);
              if (res.code != RESULT_OK)
                {
                  LOG_WARNING ("World",
                               "Failed to instantiate prefab '%s': %s",
                               tmpl->prefab_name, res.message);
                  continue;
                }
            }
          else
            {
              LOG_WARNING ("World",
                           "Prefab '%s' not found for entity template",
                           tmpl->prefab_name);
              continue;
            }
        }
      else
        {

          entity = ecs_entity_create (world);
          if (entity == INVALID_ENTITY)
            {
              return RESULT_ERROR (RESULT_ERROR_ALLOCATION,
                                   "Failed to create entity");
            }
        }

      for (size_t j = 0; j < tmpl->component_count; j++)
        {
          const char *comp_name = tmpl->components[j].component_name;
          const void *comp_data = tmpl->components[j].data;

          component_id_t comp_id = ecs_get_component_id (world, comp_name);
          if (comp_id == INVALID_ENTITY)
            {
              LOG_WARNING ("World", "Component '%s' not registered",
                           comp_name);
              continue;
            }

          void *existing = ecs_get_component (world, entity, comp_id);
          if (existing)
            {

              if (tmpl->components[j].sexp)
                {

                  scheme_state_t *world_state
                      = world_loader_get_scheme_state ();
                  if (world_state)
                    {
                      apply_component_override (world_state, comp_name,
                                                tmpl->components[j].sexp,
                                                existing);
                    }
                  else
                    {
                      LOG_WARNING (
                          "World",
                          "Failed to get world scheme state for override");
                    }
                }
              else if (comp_data)
                {

                  memcpy (existing, comp_data, tmpl->components[j].data_size);
                }
            }
          else
            {

              if (comp_data)
                {
                  result_t result
                      = ecs_add_component (world, entity, comp_id, comp_data);
                  if (result.code != RESULT_OK)
                    {
                      LOG_WARNING ("World", "Failed to add component '%s': %s",
                                   comp_name, result.message);
                    }
                }
              else
                {
                  LOG_WARNING ("World", "No component data for '%s'",
                               comp_name);
                }
            }
        }
    }

  return RESULT_SUCCESS;
}

result_t
world_switch (world_manager_t *manager, const world_definition_t *definition,
              prefab_system_t *prefab_system)
{

  return world_load (manager, definition, prefab_system);
}

result_t
world_update (world_manager_t *manager, float delta_time)
{
  if (!manager || !manager->active_world)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER, "No active world");
    }

  ecs_world_t *world = manager->active_world;

  world->time.delta_time = delta_time;
  world->time.current_time += delta_time;
  world->time.frame_count++;

  return ecs_system_update (world);
}
