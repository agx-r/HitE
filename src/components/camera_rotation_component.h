#ifndef HITE_CAMERA_ROTATION_COMPONENT_H
#define HITE_CAMERA_ROTATION_COMPONENT_H

#include "../core/ecs.h"
#include "../core/events.h"
#include "../core/types.h"

typedef struct
{

  float yaw;
  float pitch;

  float look_sensitivity;
  bool first_mouse;
  double last_mouse_x;
  double last_mouse_y;

  float max_pitch;
  float min_pitch;

  bool mouse_captured;
  bool enabled;

  listener_id_t mouse_move_listener;
} ALIGN_64 camera_rotation_component_t;

void camera_rotation_component_register (ecs_world_t *world);

camera_rotation_component_t camera_rotation_create_default (float yaw,
                                                            float pitch);

#endif
