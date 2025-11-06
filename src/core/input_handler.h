#ifndef HITE_INPUT_HANDLER_H
#define HITE_INPUT_HANDLER_H

#include "events.h"
#include "types.h"

#include <GLFW/glfw3.h>

typedef struct
{
  event_system_t *event_system;
  GLFWwindow *window;
} input_handler_t;

result_t input_handler_init (input_handler_t *handler,
                             event_system_t *event_system, GLFWwindow *window);
void input_handler_cleanup (input_handler_t *handler);

#endif
