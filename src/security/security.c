#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "security.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/*============================================================================
 * Input validation
 *============================================================================*/
int security_validate_string(const char* str, size_t max_len) {
    if (!str) return 0;
    if (strlen(str) > max_len) return 0;
    return security_check_input(str, strlen(str));
}

int security_check_input(const char* data, size_t len) {
    if (!data) return 0;
    if (len == 0) len = strlen(data);

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)data[i];

        /* Reject NUL byte */
        if (c == '\0') return 0;

        /* Reject control characters except newline, tab, carriage return */
        if (c < 32 && c != '\n' && c != '\t' && c != '\r') return 0;

        /* Reject DEL */
        if (c == 127) return 0;
    }
    return 1;
}

int security_validate_path(const char* path) {
    if (!path || *path == '\0') return 0;

    /* Must start with / */
    if (*path != '/') return 0;

    /* Check for dangerous patterns */
    const char* p = path;
    while (*p) {
        /* No parent directory references */
        if (p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == '\0')) {
            return 0;
        }
        /* No null bytes */
        if (*p == '\0') break;
        p++;
    }

    /* Path too long? */
    if (strlen(path) > 4096) return 0;

    return 1;
}

/*============================================================================
 * SQL escaping
 *============================================================================*/
char* security_escape_string(const char* input, size_t input_len) {
    if (!input) return NULL;
    if (input_len == 0) input_len = strlen(input);

    /* Each character can become at most 2 (') */
    char* output = malloc(input_len * 2 + 1);
    if (!output) return NULL;

    char* dest = output;
    for (size_t i = 0; i < input_len; i++) {
        if (input[i] == '\'') {
            *dest++ = '\'';
            *dest++ = '\'';
        } else {
            *dest++ = input[i];
        }
    }
    *dest = '\0';

    return output;
}

void security_escape_free(char* escaped) {
    free(escaped);
}

/*============================================================================
 * File permissions
 *============================================================================*/
static int set_permissions(const char* path, mode_t mode) {
    if (!path) return -1;

    struct stat st;
    if (stat(path, &st) != 0) {
        /* Directory might not exist yet - try to create */
        return -1;
    }

    if (chmod(path, mode) != 0) {
        return -1;
    }

    return 0;
}

int security_set_data_dir_permissions(const char* path) {
    /* Data directory: 0750 (owner rwx, group r-x, other none) */
    return set_permissions(path, 0750);
}

int security_set_run_dir_permissions(const char* path) {
    /* Run directory: 0755 (owner rwx, group r-x, other r-x) */
    return set_permissions(path, 0755);
}

int security_set_socket_permissions(const char* path) {
    /* Socket: 0666 (owner rw, group rw, other rw) */
    return set_permissions(path, 0666);
}

/*============================================================================
 * Connection security
 *============================================================================*/
void security_set_default_limits(ConnectionLimits* limits) {
    if (!limits) return;
    limits->max_connections = 64;
    limits->max_concurrent_queries = 4;
    limits->query_timeout_seconds = 300;
}

int security_check_connection_limit(const ConnectionLimits* limits, int current_count) {
    if (!limits) return 1;  /* No limits set - allow */
    return current_count < limits->max_connections;
}

int security_check_query_timeout(const ConnectionLimits* limits, int elapsed_seconds) {
    if (!limits) return 1;  /* No limits set - allow */
    return elapsed_seconds < limits->query_timeout_seconds;
}

/*============================================================================
 * SQL injection detection
 *============================================================================*/
int security_check_sql_suspicious(const char* sql, size_t len) {
    if (!sql || len == 0) return 0;
    if (len > TINYDB_MAX_SQL_STATEMENT_LENGTH) return 0;

    /* Look for suspicious patterns */

    /* -- comment at start (inline SQL injection) */
    if (len >= 2 && sql[0] == '-' && sql[1] == '-') return 1;

    /* ; (statement terminator followed by another statement) */
    for (size_t i = 0; i < len; i++) {
        if (sql[i] == ';') {
            /* Check if there's meaningful content after the semicolon */
            size_t j = i + 1;
            while (j < len && isspace((unsigned char)sql[j])) j++;
            if (j < len && isalpha((unsigned char)sql[j])) {
                return 1;  /* Suspicious: another statement follows */
            }
        }
    }

    /* UNION, SELECT, INSERT, UPDATE, DELETE, DROP, CREATE, ALTER, exec, etc. */
    static const char* keywords[] = {
        "union", "select", "insert", "update", "delete",
        "drop", "create", "alter", "exec", "execute",
        "script", "javascript", "onerror", "onload"
    };

    /* Case-insensitive search for suspicious keywords */
    for (size_t i = 0; i < len - 2; i++) {
        for (size_t k = 0; k < sizeof(keywords)/sizeof(keywords[0]); k++) {
            size_t kw_len = strlen(keywords[k]);
            if (i + kw_len <= len) {
                int match = 1;
                for (size_t j = 0; j < kw_len; j++) {
                    char c1 = sql[i + j];
                    char c2 = keywords[k][j];
                    if (tolower((unsigned char)c1) != c2) {
                        match = 0;
                        break;
                    }
                }
                if (match && (i == 0 || !isalnum((unsigned char)sql[i-1])) &&
                    (i + kw_len >= len || !isalnum((unsigned char)sql[i + kw_len]))) {
                    /* Check it's not part of a legitimate word */
                    return 1;
                }
            }
        }
    }

    return 0;  /* Not suspicious */
}

/*============================================================================
 * Security initialization
 *============================================================================*/
void security_init(void) {
    /* Currently a placeholder for future security initialization */
    /* Could initialize OpenSSL, load security policies, etc. */
}

void security_shutdown(void) {
    /* Currently a placeholder for future security cleanup */
}