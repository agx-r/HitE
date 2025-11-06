#include "input_handler.h"
#include "global.h"
#include "logger.h"

#include <GLFW/glfw3.h>

static void
key_callback (GLFWwindow *window, int key, int scancode, int action, int mods)
{
  (void)scancode;

  engine_state_t *state = (engine_state_t *)glfwGetWindowUserPointer (window);

  if (!state)
    return;

  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
      state->running = false;
      return;
    }

  if (!state->event_system)
    return;

  event_type_t type
      = (action == GLFW_PRESS) ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE;
  event_t event = event_key_create (type, key, mods);
  event_broadcast (state->event_system, &event);
}

static void
mouse_callback (GLFWwindow *window, double xpos, double ypos)
{

  engine_state_t *state = (engine_state_t *)glfwGetWindowUserPointer (window);

  if (!state || !state->event_system)
    return;

  event_t event = event_mouse_move_create ((float)xpos, (float)ypos);
  event_broadcast (state->event_system, &event);
}

result_t
input_handler_init (input_handler_t *handler, event_system_t *event_system,
                    GLFWwindow *window)
{
  if (!handler || !event_system || !window)
    {
      return RESULT_ERROR (RESULT_ERROR_INVALID_PARAMETER,
                           "Invalid parameters");
    }

  handler->event_system = event_system;
  handler->window = window;

  glfwSetKeyCallback (window, key_callback);
  glfwSetCursorPosCallback (window, mouse_callback);

  LOG_INFO ("InputHandler", "Input handler initialized");

  return RESULT_SUCCESS;
}

void
input_handler_cleanup (input_handler_t *handler)
{
  if (!handler || !handler->window)
    return;

  glfwSetKeyCallback (handler->window, NULL);
  glfwSetCursorPosCallback (handler->window, NULL);
}

bool
input_handler_get_key_state (const input_handler_t *handler, int key)
{
  if (!handler || !handler->window)
    return false;

  return glfwGetKey (handler->window, key) == GLFW_PRESS;
}
