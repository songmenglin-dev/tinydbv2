#define _POSIX_C_SOURCE 200809L

#include "../../src/storage/wal.h"
#include "mini_test.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char test_db_path[512];

/* Setup and teardown */
static void setup(void) {
    snprintf(test_db_path, sizeof(test_db_path), "/tmp/tinydb_test_wal_%d.db", getpid());
}

static void teardown(void) {
    /* Clean up WAL file */
    char wal_path[520];
    snprintf(wal_path, sizeof(wal_path), "%s-wal", test_db_path);
    unlink(wal_path);
    unlink(test_db_path);
}

/* Test WAL structure creation */
test(wal_creation) {
    setup();

    WAL* wal = wal_open(test_db_path);
    assert_non_null(wal);
    assert_true(wal->is_open);
    assert_true(wal->fd >= 0);

    wal_close(wal);
    teardown();
}

/* Test WAL path generation */
test(wal_path_generation) {
    char* path = wal_get_path("/tmp/test.db");
    assert_non_null(path);
    assert_str_eq(path, "/tmp/test.db-wal");
    free(path);
}

/* Test WAL checksum calculation */
test(wal_checksum) {
    const char* data = "test data for checksum";
    uint32_t sum = wal_checksum(data, strlen(data));
    assert_true(sum != 0);

    /* Same data should produce same checksum */
    uint32_t sum2 = wal_checksum(data, strlen(data));
    assert_eq_uint64(sum, sum2);

    /* Different data should produce different checksum */
    uint32_t sum3 = wal_checksum("different data", 14);
    assert_true(sum != sum3);
}

/* Test WAL entry append */
test(wal_entry_append) {
    setup();

    WAL* wal = wal_open(test_db_path);
    assert_non_null(wal);

    /* Begin transaction */
    int ret = wal_begin_tx(wal);
    assert_eq_int(ret, SUCCESS);

    /* Commit transaction */
    ret = wal_commit_tx(wal);
    assert_eq_int(ret, SUCCESS);

    wal_close(wal);
    teardown();
}

/* Test WAL page write */
test(wal_page_write) {
    setup();

    WAL* wal = wal_open(test_db_path);
    assert_non_null(wal);

    /* Write a page */
    char page_data[PAGE_SIZE];
    memset(page_data, 'A', PAGE_SIZE);

    int ret = wal_write_page(wal, 1, 1, page_data);
    assert_eq_int(ret, SUCCESS);
    assert_true(wal->frame_count > 0);

    wal_close(wal);
    teardown();
}

/* Test WAL flush */
test(wal_flush) {
    setup();

    WAL* wal = wal_open(test_db_path);
    assert_non_null(wal);

    /* Write something */
    wal_begin_tx(wal);
    char page_data[PAGE_SIZE];
    memset(page_data, 'B', PAGE_SIZE);
    wal_write_page(wal, 1, 1, page_data);

    /* Flush */
    int ret = wal_flush(wal);
    assert_eq_int(ret, SUCCESS);

    wal_close(wal);
    teardown();
}

/* Test WAL close and reopen */
test(wal_reopen) {
    setup();

    /* Create WAL and write something */
    {
        WAL* wal = wal_open(test_db_path);
        wal_begin_tx(wal);
        char page_data[PAGE_SIZE];
        memset(page_data, 'C', PAGE_SIZE);
        wal_write_page(wal, 1, 1, page_data);
        wal_close(wal);
    }

    /* Reopen and verify */
    {
        WAL* wal = wal_open(test_db_path);
        assert_non_null(wal);
        assert_true(wal->is_open);
        wal_close(wal);
    }

    teardown();
}

/* Run all tests */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("Running WAL tests...\n");

    run(wal_creation);
    run(wal_path_generation);
    run(wal_checksum);
    run(wal_entry_append);
    run(wal_page_write);
    run(wal_flush);
    run(wal_reopen);

    printf("\nAll WAL tests passed!\n");
    return 0;
}