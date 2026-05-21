#ifndef TINYDB_SECURITY_H
#define TINYDB_SECURITY_H

#include <stdbool.h>
#include <stddef.h>

/*============================================================================
 * Input validation
 *============================================================================*/

/* Validate a string input for dangerous characters
 * Returns: 1 if valid, 0 if invalid */
int security_validate_string(const char* str, size_t max_len);

/* Validate a path - must be absolute, no .. or relative components
 * Returns: 1 if valid, 0 if invalid */
int security_validate_path(const char* path);

/* Check for NUL bytes, control characters (except newline, tab, CR)
 * Returns: 1 if clean, 0 if dangerous */
int security_check_input(const char* data, size_t len);

/* Escape a string for SQL - replaces ' with ''
 * Returns: allocated buffer with escaped string (caller must free)
 * Returns NULL on allocation failure */
char* security_escape_string(const char* input, size_t input_len);

/* Free escaped string */
void security_escape_free(char* escaped);

/*============================================================================
 * File permissions
 *============================================================================*/

/* Set secure permissions on data directory (0750)
 * Returns: 0 on success, -1 on error */
int security_set_data_dir_permissions(const char* path);

/* Set secure permissions on run directory (0755)
 * Returns: 0 on success, -1 on error */
int security_set_run_dir_permissions(const char* path);

/* Set secure permissions on socket file (0666)
 * Returns: 0 on success, -1 on error */
int security_set_socket_permissions(const char* path);

/*============================================================================
 * Connection security
 *============================================================================*/
typedef struct {
    int max_connections;         /* maximum concurrent connections */
    int max_concurrent_queries;  /* maximum concurrent queries per connection */
    int query_timeout_seconds;   /* query timeout in seconds */
} ConnectionLimits;

/* Set default connection limits */
void security_set_default_limits(ConnectionLimits* limits);

/* Validate connection count
 * Returns: 1 if allowed, 0 if rejected */
int security_check_connection_limit(const ConnectionLimits* limits, int current_count);

/* Validate query timeout
 * Returns: 1 if allowed, 0 if timeout exceeded */
int security_check_query_timeout(const ConnectionLimits* limits, int elapsed_seconds);

/*============================================================================
 * SQL injection prevention helpers
 *============================================================================*/

/* Check if a SQL fragment looks suspicious */
int security_check_sql_suspicious(const char* sql, size_t len);

/* Maximum SQL statement length (1MB) */
#define TINYDB_MAX_SQL_STATEMENT_LENGTH (1024 * 1024)

/*============================================================================
 * Security initialization
 *============================================================================*/

/* Initialize security subsystem
 * Call once at server startup */
void security_init(void);

/* Cleanup security subsystem */
void security_shutdown(void);

#endif /* TINYDB_SECURITY_H */