#ifndef HITE_CAMERA_ROTATION_COMPONENT_H
#define HITE_CAMERA_ROTATION_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

// Camera rotation component - handles mouse look
typedef struct
{
  // Rotation angles
  float yaw;   // Horizontal rotation
  float pitch; // Vertical rotation

  // Mouse input state
  float look_sensitivity;
  bool first_mouse;
  double last_mouse_x;
  double last_mouse_y;

  // Constraints
  float max_pitch;
  float min_pitch;

  // Flags
  bool mouse_captured;
  bool enabled;
} ALIGN_64 camera_rotation_component_t;

// Component lifecycle
result_t camera_rotation_component_start (ecs_world_t *world,
                                          entity_id_t entity,
                                          void *component_data);
result_t camera_rotation_component_update (ecs_world_t *world,
                                           entity_id_t entity,
                                           void *component_data,
                                           const time_info_t *time);
void camera_rotation_component_destroy (void *component_data);

// Register component
void camera_rotation_component_register (ecs_world_t *world);

// Helper functions
void camera_rotation_process_mouse (camera_rotation_component_t *rotation,
                                    double xpos, double ypos);
camera_rotation_component_t camera_rotation_create_default (float yaw,
                                                            float pitch);

#endif // HITE_CAMERA_ROTATION_COMPONENT_H
