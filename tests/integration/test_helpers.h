#ifndef TINYDB_TEST_HELPERS_H
#define TINYDB_TEST_HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

/*============================================================================
 * Test infrastructure for TinyDB integration tests
 *============================================================================*/

/* Test database creation and teardown */
typedef struct TestDb {
    char* db_path;
    char* socket_path;
    pid_t server_pid;
} TestDb;

static TestDb* test_db_create(const char* name) {
    TestDb* db = calloc(1, sizeof(TestDb));
    if (!db) return NULL;

    db->db_path = malloc(256);
    db->socket_path = malloc(256);

    snprintf(db->db_path, 256, "/tmp/tinydb_test_%s.db", name);
    snprintf(db->socket_path, 256, "/tmp/tinydb_test_%s.sock", name);

    return db;
}

static void test_db_free(TestDb* db) {
    if (!db) return;

    if (db->server_pid > 0) {
        kill(db->server_pid, SIGTERM);
        waitpid(db->server_pid, NULL, 0);
    }

    unlink(db->db_path);
    unlink(db->socket_path);

    free(db->db_path);
    free(db->socket_path);
    free(db);
}

static int test_db_send_query(const char* socket_path, const char* query,
                              char* response, size_t response_size) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    ssize_t len = strlen(query);
    if (write(fd, query, len) != len) {
        close(fd);
        return -1;
    }

    /* Read response */
    ssize_t total = 0;
    ssize_t n;
    while ((n = read(fd, response + total, response_size - total - 1)) > 0) {
        total += n;
        if (total >= (ssize_t)(response_size - 1)) break;
    }
    response[total] = '\0';

    close(fd);
    return 0;
}

/* Test assertion macros for integration tests */
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s\n", msg); \
        return 1; \
    } \
} while(0)

#define TEST_ASSERT_STR_CONTAINS(haystack, needle) do { \
    if (!strstr(haystack, needle)) { \
        fprintf(stderr, "FAIL: '%s' does not contain '%s'\n", haystack, needle); \
        return 1; \
    } \
} while(0)

/* Macro to define an integration test */
#define INTEGRATION_TEST(name) int test_##name(void)

/* Run an integration test and report result */
#define RUN_INTEGRATION_TEST(name) do { \
    printf("  %s... ", #name); \
    fflush(stdout); \
    if (test_##name() == 0) { \
        printf("OK\n"); \
    } else { \
        printf("FAIL\n"); \
        failed++; \
    } \
} while(0)

#endif /* TINYDB_TEST_HELPERS_H */