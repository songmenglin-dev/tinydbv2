#include "error.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/* Global panic state */
volatile PanicInfo g_panic = { PANIC_NONE, NULL, NULL, 0 };

/*============================================================================
 * Error code to string conversion
 *============================================================================*/
const char* error_code_to_string(ErrorCode code) {
    switch (code) {
        case SUCCESS:
            return "Success";
        /* Parse errors */
        case ERR_PARSE_SYNTAX:
            return "Syntax error";
        case ERR_PARSE_UNEXPECTED_TOKEN:
            return "Unexpected token";
        case ERR_PARSE_UNTERMINATED_STRING:
            return "Unterminated string";
        case ERR_PARSE_INVALID_NUMBER:
            return "Invalid number";
        case ERR_PARSE_INVALID_IDENTIFIER:
            return "Invalid identifier";
        /* Execution errors */
        case ERR_EXEC_TABLE_NOT_FOUND:
            return "Table not found";
        case ERR_EXEC_COLUMN_NOT_FOUND:
            return "Column not found";
        case ERR_EXEC_DUPLICATE_COLUMN:
            return "Duplicate column name";
        case ERR_EXEC_TYPE_MISMATCH:
            return "Type mismatch";
        case ERR_EXEC_NOT_NULL_VIOLATION:
            return "NOT NULL constraint violation";
        case ERR_EXEC_CONSTRAINT_VIOLATION:
            return "Constraint violation";
        case ERR_EXEC_FOREIGN_KEY_VIOLATION:
            return "Foreign key violation";
        case ERR_EXEC_UNIQUE_VIOLATION:
            return "UNIQUE constraint violation";
        /* Transaction errors */
        case ERR_TX_ALREADY_ACTIVE:
            return "Transaction already active";
        case ERR_TX_NOT_ACTIVE:
            return "No active transaction";
        case ERR_TX_LOCK_TIMEOUT:
            return "Lock timeout";
        case ERR_TX_DEADLOCK:
            return "Deadlock detected";
        case ERR_TX_ROLLBACK:
            return "Transaction rolled back";
        /* Storage errors */
        case ERR_STORAGE_IO:
            return "I/O error";
        case ERR_STORAGE_CORRUPT:
            return "Database file corrupt";
        case ERR_STORAGE_FULL:
            return "Database file full";
        case ERR_STORAGE_PERMISSION:
            return "Permission denied";
        case ERR_STORAGE_NOT_FOUND:
            return "File not found";
        /* Protocol errors */
        case ERR_PROTO_INVALID_REQUEST:
            return "Invalid request";
        case ERR_PROTO_CONNECTION_CLOSED:
            return "Connection closed";
        case ERR_PROTO_TIMEOUT:
            return "Protocol timeout";
        /* Internal errors */
        case ERR_INTERNAL:
            return "Internal error";
        case ERR_OUT_OF_MEMORY:
            return "Out of memory";
        case ERR_ASSERTION_FAILED:
            return "Assertion failed";
        default:
            return "Unknown error";
    }
}

/*============================================================================
 * Error creation
 *============================================================================*/
Error* error_create(ErrorCode code, const char* message) {
    Error* err = malloc(sizeof(Error));
    if (err) {
        err->code = code;
        if (message) {
            strncpy(err->message, message, sizeof(err->message) - 1);
            err->message[sizeof(err->message) - 1] = '\0';
        } else {
            err->message[0] = '\0';
        }
        err->context[0] = '\0';
        err->file = NULL;
        err->line = 0;
    }
    return err;
}

Error* error_create_f(ErrorCode code, const char* fmt, ...) {
    Error* err = error_create(code, NULL);
    if (err && fmt) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(err->message, sizeof(err->message), fmt, args);
        va_end(args);
    }
    return err;
}

Error* error_wrap(Error* cause, const char* fmt, ...) {
    if (!cause) return NULL;

    Error* err = error_create(cause->code, cause->message);
    if (err && fmt) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(err->context, sizeof(err->context), fmt, args);
        va_end(args);
    }
    /* Note: we could chain errors here but keeping it simple for now */
    free(cause);
    return err;
}

/*============================================================================
 * Error comparison
 *============================================================================*/
int error_is(Error* err, ErrorCode code) {
    return err && err->code == code;
}

/*============================================================================
 * Panic handling
 *============================================================================*/
void panic_at(const char* file, int line, const char* fmt, ...) {
    g_panic.file = file;
    g_panic.line = line;

    va_list args;
    va_start(args, fmt);

    if (fmt) {
        char message[256];
        vsnprintf(message, sizeof(message), fmt, args);
        g_panic.message = message;

        fprintf(stderr, "PANIC: %s at %s:%d\n", message, file, line);
    } else {
        g_panic.message = "Unknown panic";
        fprintf(stderr, "PANIC at %s:%d\n", file, line);
    }

    va_end(args);

    g_panic.type = PANIC_ASSERTION;

    /* In debug builds, abort rather than exit to halt immediately */
#if defined(ENABLE_ASSERT) || defined(DEBUG)
    abort();
#else
    exit(1);
#endif
}

void panic(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (fmt) {
        char message[256];
        vsnprintf(message, sizeof(message), fmt, args);
        g_panic.message = message;

        fprintf(stderr, "PANIC: %s\n", message);
    }

    va_end(args);

    g_panic.type = PANIC_ASSERTION;

#if defined(ENABLE_ASSERT) || defined(DEBUG)
    abort();
#else
    exit(1);
#endif
}

void panic_clear(void) {
    g_panic.type = PANIC_NONE;
    g_panic.message = NULL;
    g_panic.file = NULL;
    g_panic.line = 0;
}
