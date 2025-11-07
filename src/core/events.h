#ifndef HITE_EVENTS_H
#define HITE_EVENTS_H

#include "types.h"

#define MAX_EVENT_TYPES 256
#define MAX_EVENT_LISTENERS 1024
#define EVENT_QUEUE_SIZE 4096

typedef enum
{
  EVENT_NONE = 0,
  EVENT_COLLISION,
  EVENT_KEY_PRESS,
  EVENT_KEY_RELEASE,
  EVENT_MOUSE_MOVE,
  EVENT_MOUSE_BUTTON,
  EVENT_ENTITY_DESTROYED,
  EVENT_CUSTOM_START = 100,
} event_type_t;

typedef struct
{
  event_type_t type;
  entity_id_t entity;
  double timestamp;

  union
  {

    struct
    {
      entity_id_t other_entity;
      vec3_t point;
      vec3_t normal;
    } collision;

    struct
    {
      int key;
      int mods;
    } key;

    struct
    {
      float x, y;
      int button;
    } mouse;

    uint8_t custom[64];
  } data;
} event_t;

typedef void (*event_listener_fn) (const event_t *event, void *user_data);

typedef struct
{
  event_type_t type;
  event_listener_fn callback;
  void *user_data;
  entity_id_t entity_filter;
  bool active;
} event_listener_t;

typedef struct
{

  event_t *queue;
  size_t queue_head;
  size_t queue_tail;
  size_t queue_count;

  event_listener_t *listeners;
  size_t listener_count;
  size_t listener_capacity;
} event_system_t;

event_system_t *event_system_create (void);
void event_system_destroy (event_system_t *system);

result_t event_emit (event_system_t *system, const event_t *event);
result_t event_broadcast (event_system_t *system, const event_t *event);
void event_process (event_system_t *system);

typedef uint32_t listener_id_t;
listener_id_t event_listen (event_system_t *system, event_type_t type,
                            event_listener_fn callback, void *user_data);
listener_id_t event_listen_entity (event_system_t *system, event_type_t type,
                                   entity_id_t entity,
                                   event_listener_fn callback,
                                   void *user_data);
void event_unlisten (event_system_t *system, listener_id_t listener_id);

static inline event_t
event_key_create (event_type_t type, int key, int mods)
{
  event_t event = { 0 };
  event.type = type;
  event.entity = INVALID_ENTITY;
  event.data.key.key = key;
  event.data.key.mods = mods;
  return event;
}

static inline event_t
event_mouse_move_create (float x, float y)
{
  event_t event = { 0 };
  event.type = EVENT_MOUSE_MOVE;
  event.entity = INVALID_ENTITY;
  event.data.mouse.x = x;
  event.data.mouse.y = y;
  return event;
}

#endif
