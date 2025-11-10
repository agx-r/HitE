#include "player_component.h"
#include "../core/logger.h"
#include "camera_component.h"
#include "component_registry.h"

#include <string.h>

static result_t
player_component_start (ecs_world_t *world, entity_id_t entity,
                        void *component_data)
{
  player_component_t *player = (player_component_t *)component_data;

  memset (player, 0, sizeof (*player));
  player->camera_entity = INVALID_ENTITY;
  player->active = true;

  component_id_t camera_id = ecs_get_component_id (world, "camera");
  if (camera_id != INVALID_ENTITY
      && ecs_has_component (world, entity, camera_id))
    {
      player->camera_entity = entity;
    }
  else
    {
      entity_id_t camera_entity = INVALID_ENTITY;
      camera_component_t *camera = camera_find_active (world, &camera_entity);
      if (camera && camera->is_active)
        {
          player->camera_entity = camera_entity;
        }
    }

  LOG_INFO ("Player",
            "Player component started for entity %u "
            "(camera entity: %u)",
            entity, player->camera_entity);

  return RESULT_SUCCESS;
}

static result_t
player_component_update (ecs_world_t *world, entity_id_t entity,
                         void *component_data, const time_info_t *time)
{
  (void)world;
  (void)entity;
  (void)component_data;
  (void)time;
  return RESULT_SUCCESS;
}

static void
player_component_destroy (void *component_data)
{
  (void)component_data;
}

void
player_component_register (ecs_world_t *world)
{
  static const char *dependencies[] = { "camera", NULL };
  REGISTER_COMPONENT (world, "player", player_component_t,
                      player_component_start, player_component_update, NULL,
                      player_component_destroy, "Player", 64, dependencies);
}
