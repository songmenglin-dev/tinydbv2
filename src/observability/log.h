#ifndef TINYDB_LOG_H
#define TINYDB_LOG_H

#include <stdio.h>

/*============================================================================
 * Log levels
 *============================================================================*/
typedef enum {
    LOG_ERROR = 0,
    LOG_WARN = 1,
    LOG_INFO = 2,
    LOG_DEBUG = 3
} LogLevel;

/*============================================================================
 * Log destination
 *============================================================================*/
typedef enum {
    LOG_DEST_STDERR = 0,
    LOG_DEST_SYSLOG = 1,
    LOG_DEST_FILE = 2,
    LOG_DEST_JOURNALD = 3
} LogDestination;

/*============================================================================
 * Log categories for filtering
 *============================================================================*/
typedef enum {
    LOG_CAT_GENERAL = 0,
    LOG_CAT_STORAGE = 1,
    LOG_CAT_SQL = 2,
    LOG_CAT_PROTOCOL = 3,
    LOG_CAT_SECURITY = 4,
    LOG_CAT_COUNT = 5
} LogCategory;

/*============================================================================
 * Logger instance
 *============================================================================*/
typedef struct {
    LogLevel level;
    LogDestination dest;
    FILE* file;                  /* for file destination */
    int categories_enabled;      /* bitmask of enabled categories */
    int use_colors;             /* colored output to terminal */
} Logger;

/*============================================================================
 * Initialization and cleanup
 *============================================================================*/
void log_init(Logger* logger, LogDestination dest, LogLevel level);
void log_destroy(Logger* logger);

/*============================================================================
 * Logging functions
 *============================================================================*/
void log_set_level(Logger* logger, LogLevel level);
void log_enable_category(Logger* logger, LogCategory cat);
void log_disable_category(Logger* logger, LogCategory cat);

void log_write(Logger* logger, LogCategory cat, LogLevel level,
               const char* file, int line, const char* fmt, ...);

/* Rotation support */
void log_rotate(Logger* logger);
int log_set_file(Logger* logger, const char* path);

/* Convenience macros - use these in code
 * Note: These reference a global 'g_logger' - set up appropriately in server */
#define LOG_ERROR(cat, ...) \
    log_write(&g_logger, cat, LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(cat, ...) \
    log_write(&g_logger, cat, LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(cat, ...) \
    log_write(&g_logger, cat, LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(cat, ...) \
    log_write(&g_logger, cat, LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

#endif /* TINYDB_LOG_H */