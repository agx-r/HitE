#include "logger.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static log_level_t g_log_level = LOG_LEVEL_INFO;
static log_target_t g_log_target = LOG_TARGET_STDOUT;
static FILE *g_log_file = NULL;
static bool g_initialized = false;

static const char *
log_level_to_string (log_level_t level)
{
  switch (level)
    {
    case LOG_LEVEL_DEBUG:
      return "DEBUG";
    case LOG_LEVEL_INFO:
      return "INFO";
    case LOG_LEVEL_WARNING:
      return "WARNING";
    case LOG_LEVEL_ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
    }
}

static const char *
log_level_to_color (log_level_t level)
{
  switch (level)
    {
    case LOG_LEVEL_DEBUG:
      return "\033[36m";
    case LOG_LEVEL_INFO:
      return "\033[32m";
    case LOG_LEVEL_WARNING:
      return "\033[33m";
    case LOG_LEVEL_ERROR:
      return "\033[31m";
    default:
      return "\033[0m";
    }
}

static FILE *
get_log_stream (void)
{
  switch (g_log_target)
    {
    case LOG_TARGET_STDOUT:
      return stdout;
    case LOG_TARGET_STDERR:
      return stderr;
    case LOG_TARGET_FILE:
      return g_log_file ? g_log_file : stderr;
    default:
      return stderr;
    }
}

static bool
is_tty (FILE *stream)
{
  return (g_log_target != LOG_TARGET_FILE && isatty (fileno (stream)));
}

void
logger_init (void)
{
  if (g_initialized)
    return;

  g_log_level = LOG_LEVEL_INFO;
  g_log_target = LOG_TARGET_STDOUT;
  g_log_file = NULL;
  g_initialized = true;
}

void
logger_shutdown (void)
{
  if (g_log_file)
    {
      fclose (g_log_file);
      g_log_file = NULL;
    }
  g_initialized = false;
}

void
logger_set_level (log_level_t level)
{
  g_log_level = level;
}

log_level_t
logger_get_level (void)
{
  return g_log_level;
}

void
logger_log (log_level_t level, const char *module, const char *format, ...)
{
  if (!g_initialized)
    logger_init ();

  if (level < g_log_level)
    return;

  va_list args;
  va_start (args, format);
  logger_logv (level, module, format, args);
  va_end (args);
}

void
logger_logv (log_level_t level, const char *module, const char *format,
             va_list args)
{
  if (!g_initialized)
    logger_init ();

  if (level < g_log_level)
    return;

  FILE *stream = get_log_stream ();
  if (!stream)
    return;

  time_t now = time (NULL);
  struct tm *tm_info = localtime (&now);
  char time_str[64];
  strftime (time_str, sizeof (time_str), "%Y-%m-%d %H:%M:%S", tm_info);

  bool use_colors = is_tty (stream);
  const char *color = use_colors ? log_level_to_color (level) : "";
  const char *reset = use_colors ? "\033[0m" : "";

  fprintf (stream, "%s[%s] [%s] [%s]%s ", color, time_str,
           log_level_to_string (level), module ? module : "UNKNOWN", reset);

  vfprintf (stream, format, args);
  fprintf (stream, "\n");

  fflush (stream);
}
