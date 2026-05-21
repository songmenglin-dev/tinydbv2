#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "log.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>

/*============================================================================
 * Internal helpers
 *============================================================================*/
static const char* category_to_string(LogCategory cat) {
    switch (cat) {
        case LOG_CAT_GENERAL:   return "general";
        case LOG_CAT_STORAGE:  return "storage";
        case LOG_CAT_SQL:       return "sql";
        case LOG_CAT_PROTOCOL:  return "protocol";
        case LOG_CAT_SECURITY:  return "security";
        default:               return "unknown";
    }
}

static const char* level_color(int level) {
    if (level == 0) return "\033[31m";   /* ERROR */
    if (level == 1) return "\033[33m";   /* WARN */
    if (level == 2) return "\033[32m";   /* INFO */
    if (level == 3) return "\033[34m";   /* DEBUG */
    return "\033[0m";
}

static void format_timestamp(char* buf, size_t buf_size) {
    time_t now = time(NULL);
    struct tm tm_buf;
    struct tm* tm_info = localtime_r(&now, &tm_buf);
    if (tm_info) {
        strftime(buf, buf_size, "%Y-%m-%dT%H:%M:%S%z", tm_info);
    } else {
        buf[0] = '\0';
    }
}

static void write_to_stream(int level, LogCategory cat,
                            const char* file, int line,
                            const char* timestamp, const char* message,
                            int use_colors) {
    const char* color = level_color(level);
    const char* cat_str = category_to_string(cat);
    const char* reset = "\033[0m";

    if (use_colors) {
        fprintf(stderr, "%s%s%s [%s] %s:%d %s%s\n",
                color, timestamp, reset, cat_str,
                file, line, message, reset);
    } else {
        fprintf(stderr, "%s [%s] %s:%d %s\n",
                timestamp, cat_str,
                file, line, message);
    }
}

static void write_to_file(FILE* f, int level, LogCategory cat,
                          const char* file, int line,
                          const char* timestamp, const char* message,
                          int use_colors) {
    if (!f) return;

    const char* color = level_color(level);
    const char* cat_str = category_to_string(cat);
    const char* reset = "\033[0m";

    if (use_colors) {
        fprintf(f, "%s%s%s [%s] %s:%d %s%s\n",
                color, timestamp, reset, cat_str,
                file, line, message, reset);
    } else {
        fprintf(f, "%s [%s] %s:%d %s\n",
                timestamp, cat_str,
                file, line, message);
    }
    fflush(f);
}

/*============================================================================
 * Initialization
 *============================================================================*/
void log_init(Logger* logger, LogDestination dest, LogLevel level) {
    if (!logger) return;

    memset(logger, 0, sizeof(Logger));
    logger->level = level;
    logger->dest = dest;
    logger->categories_enabled = (1 << LOG_CAT_COUNT) - 1;  /* all enabled */
    logger->use_colors = 1;  /* default to colors on terminal */
}

void log_destroy(Logger* logger) {
    if (!logger) return;

    if (logger->file && logger->file != stderr) {
        fclose(logger->file);
        logger->file = NULL;
    }
}

/*============================================================================
 * Configuration
 *============================================================================*/
void log_set_level(Logger* logger, LogLevel level) {
    if (logger) logger->level = level;
}

void log_enable_category(Logger* logger, LogCategory cat) {
    if (logger && cat >= 0 && cat < LOG_CAT_COUNT) {
        logger->categories_enabled |= (1 << cat);
    }
}

void log_disable_category(Logger* logger, LogCategory cat) {
    if (logger && cat >= 0 && cat < LOG_CAT_COUNT) {
        logger->categories_enabled &= ~(1 << cat);
    }
}

int log_set_file(Logger* logger, const char* path) {
    if (!logger) return -1;

    FILE* f = fopen(path, "a");
    if (!f) return -1;

    /* Close old file if not stderr */
    if (logger->file && logger->file != stderr) {
        fclose(logger->file);
    }

    logger->file = f;
    logger->dest = LOG_DEST_FILE;
    return 0;
}

void log_rotate(Logger* logger) {
    if (!logger || !logger->file || logger->file == stderr) return;

    /* Reopen in append mode - rotation is simpler for now */
    FILE* f = fopen("/dev/null", "a");  /* placeholder - would need actual rotation */
    if (f) {
        fclose(logger->file);
        logger->file = f;
    }
}

/*============================================================================
 * Core logging function
 *============================================================================*/
void log_write(Logger* logger, LogCategory cat, LogLevel level,
               const char* file, int line, const char* fmt, ...) {
    if (!logger) return;

    /* Level filter */
    if (level > logger->level) return;

    /* Category filter */
    if (cat < 0 || cat >= LOG_CAT_COUNT ||
        !(logger->categories_enabled & (1 << cat))) return;

    /* Format the message */
    char message[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    /* Get timestamp */
    char timestamp_buf[64];
    format_timestamp(timestamp_buf, sizeof(timestamp_buf));

    /* Write to destination */
    switch (logger->dest) {
        case LOG_DEST_STDERR:
            write_to_stream(level, cat, file, line, timestamp_buf, message,
                           logger->use_colors);
            break;

        case LOG_DEST_FILE:
            if (logger->file) {
                write_to_file(logger->file, level, cat, file, line,
                             timestamp_buf, message, logger->use_colors);
            } else {
                write_to_stream(level, cat, file, line, timestamp_buf, message,
                               logger->use_colors);
            }
            break;

        case LOG_DEST_SYSLOG:
        case LOG_DEST_JOURNALD:
            /* These would require syslog - fall back to stderr */
            write_to_stream(level, cat, file, line, timestamp_buf, message,
                           logger->use_colors);
            break;
    }
}