#ifndef HITE_CAMERA_MOVEMENT_COMPONENT_H
#define HITE_CAMERA_MOVEMENT_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

// Camera movement component - handles WASD movement
typedef struct
{
  // Movement parameters
  float move_speed;

  // Input state
  bool keys[1024];

  // Flags
  bool enabled;
} ALIGN_64 camera_movement_component_t;

// Component lifecycle
result_t camera_movement_component_start (ecs_world_t *world,
                                          entity_id_t entity,
                                          void *component_data);
result_t camera_movement_component_update (ecs_world_t *world,
                                           entity_id_t entity,
                                           void *component_data,
                                           const time_info_t *time);
void camera_movement_component_destroy (void *component_data);

// Register component
void camera_movement_component_register (ecs_world_t *world);

// Helper functions
void camera_movement_process_key (camera_movement_component_t *movement,
                                  int key, int action);
camera_movement_component_t camera_movement_create_default (void);

#endif // HITE_CAMERA_MOVEMENT_COMPONENT_H
