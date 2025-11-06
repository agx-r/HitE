#ifndef HITE_LOGGER_H
#define HITE_LOGGER_H

#include <stdarg.h>
#include <stdbool.h>

typedef enum
{
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO = 1,
  LOG_LEVEL_WARNING = 2,
  LOG_LEVEL_ERROR = 3,
} log_level_t;

typedef enum
{
  LOG_TARGET_STDOUT = 0,
  LOG_TARGET_STDERR = 1,
  LOG_TARGET_FILE = 2,
} log_target_t;

void logger_init (void);
void logger_shutdown (void);

void logger_set_level (log_level_t level);
log_level_t logger_get_level (void);

void logger_set_target (log_target_t target);
void logger_set_file (const char *filepath);

void logger_log (log_level_t level, const char *module, const char *format,
                 ...);
void logger_logv (log_level_t level, const char *module, const char *format,
                  va_list args);

#define LOG_DEBUG(module, ...)                                                \
  logger_log (LOG_LEVEL_DEBUG, module, __VA_ARGS__)
#define LOG_INFO(module, ...) logger_log (LOG_LEVEL_INFO, module, __VA_ARGS__)
#define LOG_WARNING(module, ...)                                              \
  logger_log (LOG_LEVEL_WARNING, module, __VA_ARGS__)
#define LOG_ERROR(module, ...)                                                \
  logger_log (LOG_LEVEL_ERROR, module, __VA_ARGS__)

#endif
