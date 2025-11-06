#include "events.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

event_system_t *
event_system_create (void)
{
  event_system_t *system = calloc (1, sizeof (event_system_t));
  if (!system)
    return NULL;

  system->queue = calloc (EVENT_QUEUE_SIZE, sizeof (event_t));
  system->listeners = calloc (MAX_EVENT_LISTENERS, sizeof (event_listener_t));
  system->listener_capacity = MAX_EVENT_LISTENERS;

  if (!system->queue || !system->listeners)
    {
      event_system_destroy (system);
      return NULL;
    }

  return system;
}

void
event_system_destroy (event_system_t *system)
{
  if (!system)
    return;
  free (system->queue);
  free (system->listeners);
  free (system);
}

result_t
event_emit (event_system_t *system, const event_t *event)
{
  if (!system || !event)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

  if (system->queue_count >= EVENT_QUEUE_SIZE)
    {
      return RESULT_ERROR (RESULT_ERROR_ALLOCATION, "Event queue full");
    }

  system->queue[system->queue_tail] = *event;
  system->queue[system->queue_tail].timestamp
      = (double)clock () / CLOCKS_PER_SEC;

  system->queue_tail = (system->queue_tail + 1) % EVENT_QUEUE_SIZE;
  system->queue_count++;

  return RESULT_SUCCESS;
}

result_t
event_broadcast (event_system_t *system, const event_t *event)
{
  if (!system || !event)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

  event_t broadcast_event = *event;
  broadcast_event.entity = INVALID_ENTITY;
  broadcast_event.timestamp = (double)clock () / CLOCKS_PER_SEC;

  for (size_t i = 0; i < system->listener_count; i++)
    {
      event_listener_t *listener = &system->listeners[i];

      if (!listener->active)
        continue;
      if (listener->type != broadcast_event.type)
        continue;

      listener->callback (&broadcast_event, listener->user_data);
    }

  return RESULT_SUCCESS;
}

void
event_process (event_system_t *system)
{
  if (!system)
    return;

  while (system->queue_count > 0)
    {

      event_t *event = &system->queue[system->queue_head];

      for (size_t i = 0; i < system->listener_count; i++)
        {
          event_listener_t *listener = &system->listeners[i];

          if (!listener->active)
            continue;
          if (listener->type != event->type)
            continue;

          if (listener->entity_filter != INVALID_ENTITY
              && listener->entity_filter != event->entity)
            {
              continue;
            }

          listener->callback (event, listener->user_data);
        }

      system->queue_head = (system->queue_head + 1) % EVENT_QUEUE_SIZE;
      system->queue_count--;
    }
}

listener_id_t
event_listen (event_system_t *system, event_type_t type,
              event_listener_fn callback, void *user_data)
{
  return event_listen_entity (system, type, INVALID_ENTITY, callback,
                              user_data);
}

listener_id_t
event_listen_entity (event_system_t *system, event_type_t type,
                     entity_id_t entity, event_listener_fn callback,
                     void *user_data)
{
  if (!system || !callback)
    return 0;

  if (system->listener_count >= system->listener_capacity)
    {
      return 0;
    }

  listener_id_t id = system->listener_count++;
  event_listener_t *listener = &system->listeners[id];

  listener->type = type;
  listener->callback = callback;
  listener->user_data = user_data;
  listener->entity_filter = entity;
  listener->active = true;

  return id;
}

void
event_unlisten (event_system_t *system, listener_id_t listener_id)
{
  if (!system || listener_id >= system->listener_count)
    return;
  system->listeners[listener_id].active = false;
}
