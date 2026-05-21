#ifndef TINYDB_ERROR_H
#define TINYDB_ERROR_H

#include "../include/tinydb.h"

/*============================================================================
 * Error code to string conversion
 *============================================================================*/
const char* error_code_to_string(ErrorCode code);

/*============================================================================
 * Error creation helpers
 *============================================================================*/
Error* error_create(ErrorCode code, const char* message);
Error* error_create_f(ErrorCode code, const char* fmt, ...);
Error* error_wrap(Error* cause, const char* fmt, ...);

/*============================================================================
 * Error comparison
 *============================================================================*/
int error_is(Error* err, ErrorCode code);

/*============================================================================
 * Panic handling for unrecoverable errors
 *============================================================================*/
typedef enum {
    PANIC_NONE,
    PANIC_IO_ERROR,
    PANIC_CORRUPTION,
    PANIC_OUT_OF_MEMORY,
    PANIC_ASSERTION
} PanicType;

typedef struct {
    PanicType type;
    const char* message;
    const char* file;
    int line;
} PanicInfo;

extern volatile PanicInfo g_panic;

void panic(const char* fmt, ...) __attribute__((noreturn));
void panic_at(const char* file, int line, const char* fmt, ...) __attribute__((noreturn));
void panic_clear(void);

/*============================================================================
 * Assertion macro
 *============================================================================*/
#ifdef ENABLE_ASSERT
#define ASSERT(cond, msg) \
    ((cond) ? (void)0 : \
     panic_at(__FILE__, __LINE__, "Assertion failed: %s", msg))
#else
#define ASSERT(cond, msg) ((void)0)
#endif

#endif /* TINYDB_ERROR_H */
