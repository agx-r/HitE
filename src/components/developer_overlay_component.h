#ifndef HITE_DEVELOPER_OVERLAY_COMPONENT_H
#define HITE_DEVELOPER_OVERLAY_COMPONENT_H

#include "../core/ecs.h"
#include "../core/types.h"

typedef struct
{

  entity_id_t gui_entity;

  float frame_time_accumulator;
  uint32_t frame_count;
  float current_fps;
  float fps_update_interval;

  size_t fps_text_index;
  bool fps_text_initialized;
  size_t camera_pos_text_index;
  bool camera_pos_text_initialized;

  bool enabled;
} ALIGN_64 developer_overlay_component_t;

result_t developer_overlay_component_start (ecs_world_t *world,
                                            entity_id_t entity,
                                            void *component_data);
result_t developer_overlay_component_update (ecs_world_t *world,
                                             entity_id_t entity,
                                             void *component_data,
                                             const time_info_t *time);
result_t developer_overlay_component_render (ecs_world_t *world,
                                             entity_id_t entity,
                                             const void *component_data);
void developer_overlay_component_destroy (void *component_data);

void developer_overlay_component_register (ecs_world_t *world);

#endif
