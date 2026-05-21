#ifndef TINYDB_MINI_TEST_H
#define TINYDB_MINI_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*============================================================================
 * Minimal test harness for TinyDB v2
 *============================================================================*/

#define test(name) void test_##name(void)
#define run(name) do { \
    printf("  %s... ", #name); \
    fflush(stdout); \
    test_##name(); \
    printf("OK\n"); \
} while(0)

#define assert_eq(a, b) do { \
    if ((a) != (b)) { \
        printf("FAIL: %s != %s (%ld != %ld)\n", #a, #b, \
               (long)(a), (long)(b)); \
        exit(1); \
    } \
} while(0)

#define assert_eq_int(a, b) assert_eq(a, b)

#define assert_eq_uint64(a, b) do { \
    if ((uint64_t)(a) != (uint64_t)(b)) { \
        printf("FAIL: %s != %s (%lu != %lu)\n", #a, #b, \
               (unsigned long)(a), (unsigned long)(b)); \
        exit(1); \
    } \
} while(0)

#define assert_null(p) do { \
    if ((p) != NULL) { \
        printf("FAIL: %s should be NULL\n", #p); \
        exit(1); \
    } \
} while(0)

#define assert_non_null(p) do { \
    if ((p) == NULL) { \
        printf("FAIL: %s should not be NULL\n", #p); \
        exit(1); \
    } \
} while(0)

#define assert_str_eq(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        printf("FAIL: \"%s\" != \"%s\"\n", (a), (b)); \
        exit(1); \
    } \
} while(0)

#define assert_true(cond) do { \
    if (!(cond)) { \
        printf("FAIL: %s should be true\n", #cond); \
        exit(1); \
    } \
} while(0)

#define assert_false(cond) do { \
    if (cond) { \
        printf("FAIL: %s should be false\n", #cond); \
        exit(1); \
    } \
} while(0)

#define assert_mem_eq(a, b, size) do { \
    if (memcmp((a), (b), (size)) != 0) { \
        printf("FAIL: memory mismatch\n"); \
        exit(1); \
    } \
} while(0)

#endif /* TINYDB_MINI_TEST_H */
